#pragma once
/**
 * @file touch.h
 * GT911 capacitive touch-screen driver (I²C) for LVGL.
 *
 * Call touch_init() once in setup(), after display_init() and lv_init().
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Initialise the GT911 over I²C and register it as an LVGL input device. */
void touch_init(void);

#ifdef __cplusplus
}
#endif
