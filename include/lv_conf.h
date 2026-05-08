/**
 * @file lv_conf.h
 * LVGL v8 configuration for ESP32-S3 SPI LCD (ST7789, 320×240).
 *
 * ── Key fixes for "花屏" (garbled/noisy display) ──────────────────────────
 * 1. LV_COLOR_DEPTH  16  → RGB565, matches the 16-bit pixel format the
 *                          display is initialized with.
 * 2. LV_COLOR_16_SWAP 1  → Swap the two bytes of every RGB565 pixel before
 *                          DMA transfer over SPI.  On a little-endian CPU
 *                          the 16-bit color is stored as [low_byte, high_byte]
 *                          in memory, so SPI would send the low byte first.
 *                          The display expects the high byte first (RRRRRGGG
 *                          before GGGBBBBB).  Without this swap every color
 *                          comes out wrong and the whole screen looks like
 *                          random colored noise.
 * 3. LV_TICK_CUSTOM  1   → Uses Arduino millis() so lv_tick_inc() does not
 *                          need to be called manually from a timer ISR.
 * ─────────────────────────────────────────────────────────────────────────
 */

#if 1  /* Set to "1" to enable this file */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/**
 * Color depth: 1 (1 byte/px), 8 (RGB332), 16 (RGB565), 32 (ARGB8888).
 * Must be 16 for standard SPI LCD panels (RGB565).
 */
#define LV_COLOR_DEPTH 16

/**
 * Swap the two bytes of each RGB565 color word.
 *
 * WHY THIS MATTERS (root cause of 花屏 on SPI displays):
 *   LVGL stores a 16-bit RGB565 value in little-endian order:
 *     memory[0] = low byte  (GGGBBBBB)
 *     memory[1] = high byte (RRRRRGGG)
 *   When transferred to the display via SPI+DMA, the DMA engine reads
 *   memory[0] first.  The display therefore receives GGGBBBBB before
 *   RRRRRGGG — the bytes are in the wrong order and every pixel shows a
 *   completely wrong color (most of the screen looks like random noise).
 *   Setting LV_COLOR_16_SWAP to 1 makes LVGL pre-swap the bytes so the
 *   DMA sends RRRRRGGG first, which is exactly what the display needs.
 */
#define LV_COLOR_16_SWAP 1

/* Transparent background support (disable to save memory) */
#define LV_COLOR_SCREEN_TRANSP 0

/* Chroma-key color (pixels of this color are skipped when blitting images) */
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)

/*=========================
   MEMORY SETTINGS
 *=========================*/

/**
 * LVGL heap size in bytes.
 * If PSRAM is available (BOARD_HAS_PSRAM) the display buffers are placed in
 * PSRAM (see display.cpp), so the internal SRAM heap only needs to hold
 * LVGL's own objects and style data.
 */
#define LV_MEM_SIZE (64U * 1024U)

/* Use the internal allocator (no custom malloc) */
#define LV_MEM_CUSTOM 0
#define LV_MEM_ADR    0

/*====================
   HAL SETTINGS
 *====================*/

/* Display refresh period in milliseconds */
#define LV_DISP_DEF_REFR_PERIOD 30

/* Touchpad/input poll period in milliseconds */
#define LV_INDEV_DEF_READ_PERIOD 30

/**
 * Custom tick source — use Arduino millis() so there is no need to call
 * lv_tick_inc() from a separate timer interrupt.
 */
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE  <Arduino.h>
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#endif

/* Dots-per-inch (used to scale default widget sizes; adjust to taste) */
#define LV_DPI_DEF 130

/*=======================
   DRAWING / GPU
 *=======================*/

#define LV_DRAW_COMPLEX 1
#if LV_DRAW_COMPLEX
    #define LV_SHADOW_CACHE_SIZE 0
    #define LV_CIRCLE_CACHE_SIZE 4
#endif

/* No external GPU on this board */
#define LV_USE_GPU_STM32_DMA2D  0
#define LV_USE_GPU_SWM341_DMA   0
#define LV_USE_GPU_NXP_PXP      0
#define LV_USE_GPU_NXP_VG_LITE  0
#define LV_USE_GPU_SDL          0

/*====================
   LOGGING / DEBUG
 *====================*/

#define LV_USE_LOG 0
#if LV_USE_LOG
    #define LV_LOG_LEVEL    LV_LOG_LEVEL_WARN
    #define LV_LOG_PRINTF   0
#endif

#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0
#define LV_ASSERT_HANDLER_INCLUDE   <stdint.h>
#define LV_ASSERT_HANDLER           while(1);

#define LV_USE_PERF_MONITOR  0
#define LV_USE_MEM_MONITOR   0
#define LV_USE_REFR_DEBUG    0
#define LV_USE_SPRINTF       1

/*====================
   FONTS
 *====================*/

#define LV_FONT_MONTSERRAT_8   0
#define LV_FONT_MONTSERRAT_10  0
#define LV_FONT_MONTSERRAT_12  0
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_18  0
#define LV_FONT_MONTSERRAT_20  0
#define LV_FONT_MONTSERRAT_22  0
#define LV_FONT_MONTSERRAT_24  1
#define LV_FONT_MONTSERRAT_26  0
#define LV_FONT_MONTSERRAT_28  0
#define LV_FONT_MONTSERRAT_30  0
#define LV_FONT_MONTSERRAT_32  0
#define LV_FONT_MONTSERRAT_34  0
#define LV_FONT_MONTSERRAT_36  0
#define LV_FONT_MONTSERRAT_38  0
#define LV_FONT_MONTSERRAT_40  0
#define LV_FONT_MONTSERRAT_42  0
#define LV_FONT_MONTSERRAT_44  0
#define LV_FONT_MONTSERRAT_46  0
#define LV_FONT_MONTSERRAT_48  1

#define LV_FONT_MONTSERRAT_12_SUBPX      0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0
#define LV_FONT_SIMSUN_16_CJK            0
#define LV_FONT_UNSCII_8                 0
#define LV_FONT_UNSCII_16                0
#define LV_FONT_CUSTOM_DECLARE

#define LV_FONT_DEFAULT &lv_font_montserrat_14

#define LV_FONT_FMT_TXT_LARGE 0
#define LV_USE_FONT_COMPRESSED 0
#define LV_USE_FONT_SUBPX      0
#if LV_USE_FONT_SUBPX
    #define LV_FONT_SUBPX_BGR 0
#endif

/*====================
   TEXT SETTINGS
 *====================*/

#define LV_TXT_ENC                     LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS             " "
#define LV_TXT_LINE_BREAK_LONG_LEN     0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN  3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3
#define LV_TXT_COLOR_CMD               "#"
#define LV_USE_BIDI                    0
#define LV_USE_ARABIC_PERSIAN_CHARS    0

/*====================
   WIDGETS
 *====================*/

#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION 1
    #define LV_LABEL_LONG_TXT_HINT  0
#endif
#define LV_USE_LINE      1
#define LV_USE_ROLLER    1
#if LV_USE_ROLLER
    #define LV_ROLLER_INF_PAGES 7
#endif
#define LV_USE_SLIDER    1
#define LV_USE_SWITCH    1
#define LV_USE_TEXTAREA  1
#if LV_USE_TEXTAREA
    #define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500
#endif
#define LV_USE_TABLE     1

/*====================
   EXTRA COMPONENTS
 *====================*/

#define LV_USE_ANALOGCLOCK  0
#define LV_USE_CALENDAR     0
#if LV_USE_CALENDAR
    #define LV_CALENDAR_WEEK_STARTS_MONDAY 0
    #define LV_USE_CALENDAR_HEADER_ARROW    1
    #define LV_USE_CALENDAR_HEADER_DROPDOWN 1
#endif
#define LV_USE_CHART        1
#define LV_USE_COLORWHEEL   0
#define LV_USE_IMGBTN       0
#define LV_USE_KEYBOARD     0
#define LV_USE_LED          1
#define LV_USE_LIST         1
#define LV_USE_MENU         0
#define LV_USE_METER        1
#define LV_USE_MSGBOX       0
#define LV_USE_SPAN         0
#define LV_USE_SPINBOX      0
#define LV_USE_SPINNER      1
#define LV_USE_TABVIEW      1
#define LV_USE_TILEVIEW     0
#define LV_USE_WIN          0

/*------------------
 * Themes
 *-----------------*/
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK            0
    #define LV_THEME_DEFAULT_GROW            1
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif
#define LV_USE_THEME_SIMPLE 1
#define LV_USE_THEME_MONO   0

/*------------------
 * Layout
 *-----------------*/
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*------------------
 * 3rd-party libs
 *-----------------*/
#define LV_USE_FS_STDIO  0
#define LV_USE_FS_POSIX  0
#define LV_USE_FS_WIN32  0
#define LV_USE_FS_FATFS  0
#define LV_USE_PNG       0
#define LV_USE_BMP       0
#define LV_USE_SJPG      0
#define LV_USE_GIF       0
#define LV_USE_QRCODE    0
#define LV_USE_RLOTTIE   0
#define LV_USE_FFMPEG    0

/*====================
   EXAMPLES / DEMOS
 *====================*/
#define LV_BUILD_EXAMPLES       0
#define LV_USE_DEMO_WIDGETS     0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK   0
#define LV_USE_DEMO_STRESS      0
#define LV_USE_DEMO_MUSIC       0

#endif /* LV_CONF_H */
#endif /* #if 1 */
