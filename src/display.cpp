/**
 * @file display.cpp
 * ST7789 SPI LCD driver for ESP32-S3, using the Espressif esp-lcd component.
 *
 * ── Root-cause analysis of "花屏" (garbled / noisy display) ─────────────────
 *
 * Several bugs commonly combine to produce a screen that shows mostly random
 * coloured noise with only a small correct region visible:
 *
 * 1. Wrong byte order (most common cause of "all noise"):
 *    LVGL stores RGB565 colours as little-endian uint16_t in memory.
 *    Memory layout:  [low_byte=GGGBBBBB] [high_byte=RRRRRGGG]
 *    SPI DMA sends memory[0] first, so the display gets GGGBBBBB before
 *    RRRRRGGG — every pixel is a random-looking wrong colour.
 *    Fix → LV_COLOR_16_SWAP 1 in lv_conf.h (pre-swap bytes in LVGL).
 *
 * 2. Wrong colour space (RGB vs BGR):
 *    Using ESP_LCD_COLOR_SPACE_BGR initialises the panel in BGR mode, but
 *    LVGL produces RGB pixel data → R and B channels are permanently swapped.
 *    Fix → Use ESP_LCD_COLOR_SPACE_RGB to match LVGL's output.
 *
 * 3. Wrong display dimensions:
 *    If LVGL is told the display is e.g. 240 wide when it is actually 320,
 *    only 240/320 of each row is written; the rest stays uninitialised
 *    (noise).  The symptom is "only the right-hand portion looks correct".
 *    Fix → Set disp_drv.hor_res / ver_res to the true landscape dimensions.
 *
 * 4. Calling lv_disp_flush_ready() before DMA is done (double-buffer race):
 *    LVGL considers the buffer free as soon as flush_ready() is called and
 *    may start writing new pixel data into it while the old data is still
 *    being transferred via DMA → tearing / partial noise.
 *    Fix → Call lv_disp_flush_ready() only from the SPI-done interrupt
 *          (on_color_trans_done callback), not synchronously in flush_cb.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 *
 * Pin assignment (adjust to match your hardware):
 *
 *   SPI (SPI2 / HSPI)
 *     SCLK  GPIO12   BL (backlight)  GPIO46
 *     MOSI  GPIO11
 *     CS    GPIO10
 *     DC    GPIO9
 *     RST   GPIO8
 */

#include "display.h"

#include <Arduino.h>
#include <lvgl.h>

/* esp-lcd component (from espressif/esp-lcd PlatformIO library) */
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <driver/spi_master.h>
#include <esp_heap_caps.h>

/* ── Pin definitions ─────────────────────────────────────────── */
#define LCD_SCLK_PIN   12
#define LCD_MOSI_PIN   11
#define LCD_CS_PIN     10
#define LCD_DC_PIN      9
#define LCD_RST_PIN     8
#define LCD_BL_PIN     46

/* ── Display dimensions (landscape 320 × 240) ───────────────── */
#define LCD_H_RES  320
#define LCD_V_RES  240

/* ── LVGL draw-buffer: 40 lines of pixels ───────────────────── */
#define LVGL_DRAW_BUF_LINES 40

/* ──────────────────────────────────────────────────────────────
 * Module-local state
 * ──────────────────────────────────────────────────────────── */
static esp_lcd_panel_handle_t   s_panel      = NULL;
static lv_disp_draw_buf_t       s_draw_buf;
static lv_disp_drv_t            s_disp_drv;

/*
 * Two draw buffers placed in PSRAM when available (BOARD_HAS_PSRAM is set
 * in platformio.ini's build_flags).  Keeping them in PSRAM avoids eating
 * the limited internal SRAM and prevents cache-coherency issues on some
 * toolchain versions.
 */
#ifdef BOARD_HAS_PSRAM
  static lv_color_t *s_buf1 = NULL;
  static lv_color_t *s_buf2 = NULL;
#else
  static lv_color_t s_buf1[LCD_H_RES * LVGL_DRAW_BUF_LINES];
  static lv_color_t s_buf2[LCD_H_RES * LVGL_DRAW_BUF_LINES];
#endif

/* ──────────────────────────────────────────────────────────────
 * SPI-done ISR → signals LVGL that the buffer is free
 *
 * Fix #4: This callback is invoked by the DMA interrupt when the SPI
 * transfer is complete.  Only then do we tell LVGL the buffer is free,
 * preventing any race between DMA reads and LVGL writes.
 * ──────────────────────────────────────────────────────────── */
static bool IRAM_ATTR on_color_trans_done(
    esp_lcd_panel_io_handle_t    panel_io,
    esp_lcd_panel_io_event_data_t *edata,
    void                         *user_ctx)
{
    lv_disp_drv_t *drv = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(drv);
    return false;   /* no higher-priority task needs to be woken */
}

/* ──────────────────────────────────────────────────────────────
 * LVGL flush callback
 *
 * Hands the rendered rectangle to esp-lcd which starts a non-blocking
 * DMA transfer.  lv_disp_flush_ready() is called from on_color_trans_done
 * once the transfer finishes (fix #4).
 * ──────────────────────────────────────────────────────────── */
static void lvgl_flush_cb(lv_disp_drv_t *drv,
                          const lv_area_t *area,
                          lv_color_t      *color_map)
{
    /*
     * esp-lcd draw_bitmap expects (x_start, y_start, x_end+1, y_end+1)
     * i.e. the end coordinates are exclusive.
     */
    esp_lcd_panel_draw_bitmap(s_panel,
                              area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1,
                              color_map);
    /* lv_disp_flush_ready() is called by on_color_trans_done() */
}

/* ──────────────────────────────────────────────────────────────
 * Public API
 * ──────────────────────────────────────────────────────────── */
void display_init(void)
{
    /* ── 1. Backlight ─────────────────────────────────────────── */
    pinMode(LCD_BL_PIN, OUTPUT);
    digitalWrite(LCD_BL_PIN, LOW);   /* keep off until init completes */

    /* ── 2. SPI bus ───────────────────────────────────────────── */
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num     = LCD_SCLK_PIN;
    buscfg.mosi_io_num     = LCD_MOSI_PIN;
    buscfg.miso_io_num     = -1;           /* display is write-only */
    buscfg.quadwp_io_num   = -1;
    buscfg.quadhd_io_num   = -1;
    buscfg.max_transfer_sz = LCD_H_RES * LVGL_DRAW_BUF_LINES
                             * sizeof(lv_color_t);
    ESP_ERROR_CHECK(
        spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    /* ── 3. Panel I/O (SPI) ───────────────────────────────────── */
    esp_lcd_panel_io_handle_t     io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num       = LCD_CS_PIN;
    io_config.dc_gpio_num       = LCD_DC_PIN;
    io_config.spi_clock_hz      = 26 * 1000 * 1000; /* 26 MHz — safe default for most ST7789 modules.
                                                        * Many modules work up to 40 MHz, but some exhibit
                                                        * noise artifacts above 27 MHz on long wires.
                                                        * Reduce to 20 MHz if you see intermittent glitches. */
    io_config.lcd_cmd_bits      = 8;
    io_config.lcd_param_bits    = 8;
    /*
     * Fix #4: register the DMA-done callback here, passing a pointer to
     * the driver struct as user_ctx so flush_ready() can be called from
     * the ISR without a global variable.
     */
    io_config.on_color_trans_done = on_color_trans_done;
    io_config.user_ctx            = &s_disp_drv;

    ESP_ERROR_CHECK(
        esp_lcd_new_panel_io_spi(
            (esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));

    /* ── 4. LCD panel (ST7789) ────────────────────────────────── */
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = LCD_RST_PIN;
    /*
     * Fix #2: Use RGB colour space.
     * ESP_LCD_COLOR_SPACE_BGR would set MADCTL bit 3 so the panel reorders
     * R and B channels.  Since LVGL produces standard RGB pixel data,
     * using BGR here permanently swaps red and blue → wrong colours.
     */
    panel_config.color_space    = ESP_LCD_COLOR_SPACE_RGB;
    /*
     * Fix #1 (hardware side): 16-bit pixel format (RGB565).
     * This must match LV_COLOR_DEPTH 16 in lv_conf.h.
     * Using 18 (RGB666) with 16-bit data causes colour-band noise because
     * the panel misinterprets the byte boundaries.
     */
    panel_config.bits_per_pixel = 16;

    ESP_ERROR_CHECK(
        esp_lcd_new_panel_st7789(io_handle, &panel_config, &s_panel));

    /* ── 5. Hardware reset + panel init sequence ──────────────── */
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));

    /*
     * Fix #3: Set the correct landscape orientation so that
     * hor_res = 320 and ver_res = 240 match the physical pixels.
     * swap_xy=true rotates the frame buffer 90°; mirror_x corrects the
     * resulting left-right flip for a natural landscape view.
     * Without this, LVGL draws a 320-wide image onto what the display
     * treats as a 240-wide (portrait) frame → only part of the image is
     * visible and the rest is uninitialised noise.
     */
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel, true, false));

    /* Some ST7789 variants have a pixel offset in certain rotations */
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(s_panel, 0, 0));

    /* Turn the display on */
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    /* ── 6. Allocate LVGL draw buffers ───────────────────────── */
    const size_t buf_bytes = LCD_H_RES * LVGL_DRAW_BUF_LINES
                             * sizeof(lv_color_t);
#ifdef BOARD_HAS_PSRAM
    s_buf1 = (lv_color_t *)heap_caps_malloc(buf_bytes,
                                             MALLOC_CAP_SPIRAM |
                                             MALLOC_CAP_8BIT);
    s_buf2 = (lv_color_t *)heap_caps_malloc(buf_bytes,
                                             MALLOC_CAP_SPIRAM |
                                             MALLOC_CAP_8BIT);
    if (!s_buf1 || !s_buf2) {
        /* Fall back to internal RAM if PSRAM allocation fails */
        if (s_buf1) { free(s_buf1); }
        if (s_buf2) { free(s_buf2); }
        s_buf1 = (lv_color_t *)heap_caps_malloc(buf_bytes,
                                                  MALLOC_CAP_INTERNAL |
                                                  MALLOC_CAP_8BIT);
        s_buf2 = NULL; /* single buffer fallback */
    }
    lv_disp_draw_buf_init(&s_draw_buf, s_buf1, s_buf2,
                          LCD_H_RES * LVGL_DRAW_BUF_LINES);
#else
    lv_disp_draw_buf_init(&s_draw_buf, s_buf1, s_buf2,
                          LCD_H_RES * LVGL_DRAW_BUF_LINES);
#endif

    /* ── 7. Register LVGL display driver ─────────────────────── */
    lv_disp_drv_init(&s_disp_drv);
    /*
     * Fix #3: hor_res and ver_res must equal the true landscape pixel
     * dimensions.  A mismatch here is the direct cause of partial-screen
     * rendering where only a portion of the display contains valid UI.
     */
    s_disp_drv.hor_res  = LCD_H_RES;
    s_disp_drv.ver_res  = LCD_V_RES;
    s_disp_drv.flush_cb = lvgl_flush_cb;
    s_disp_drv.draw_buf = &s_draw_buf;
    lv_disp_drv_register(&s_disp_drv);

    /* ── 8. Enable backlight ──────────────────────────────────── */
    digitalWrite(LCD_BL_PIN, HIGH);
}
