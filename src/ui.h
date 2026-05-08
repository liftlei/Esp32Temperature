#pragma once
/**
 * @file ui.h
 * Temperature / humidity display UI built with LVGL 8.
 *
 * Call ui_init() once after display_init() and touch_init().
 * Call ui_update_sensor(temperature, humidity) whenever new sensor
 * readings are available.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Create the temperature display UI on the default LVGL display. */
void ui_init(void);

/**
 * Update the sensor readings shown on screen.
 *
 * @param temperature  Temperature in °C (e.g. 25.4).
 * @param humidity     Relative humidity in % (e.g. 60.0).
 */
void ui_update_sensor(float temperature, float humidity);

#ifdef __cplusplus
}
#endif
