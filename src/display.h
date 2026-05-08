#pragma once
/**
 * @file display.h
 * SPI LCD (ST7789) initialisation via the Espressif esp-lcd component.
 *
 * Required initialisation order in setup():
 *   1. lv_init()       — must be called first (LVGL internal state)
 *   2. display_init()  — registers the LVGL display driver
 *   3. touch_init()    — registers the LVGL input device
 *   4. ui_init()       — creates LVGL widgets
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Initialise the SPI LCD and register it as the LVGL display driver. */
void display_init(void);

#ifdef __cplusplus
}
#endif
