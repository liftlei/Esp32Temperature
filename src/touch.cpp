/**
 * @file touch.cpp
 * GT911 capacitive touch-screen driver for LVGL on ESP32-S3.
 *
 * The Adafruit GT911 library handles low-level I²C communication.
 * This module wraps it into an LVGL input-device driver.
 *
 * Pin assignment (adjust to match your hardware):
 *   I²C SDA  GPIO6
 *   I²C SCL  GPIO7
 *   INT       GPIO5
 *   RST       GPIO4
 */

#include "touch.h"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GT911.h>
#include <lvgl.h>

/* ── Pin definitions ─────────────────────────────────────────── */
#define TOUCH_SDA_PIN   6
#define TOUCH_SCL_PIN   7
#define TOUCH_INT_PIN   5
#define TOUCH_RST_PIN   4

/* ── Touch-screen dimensions (must match the display resolution) */
#define TOUCH_MAX_X  320
#define TOUCH_MAX_Y  240

/* ──────────────────────────────────────────────────────────────
 * Module-local state
 * ──────────────────────────────────────────────────────────── */
static Adafruit_GT911 s_touch;
static lv_indev_drv_t s_indev_drv;

/* ──────────────────────────────────────────────────────────────
 * LVGL input-device read callback
 * ──────────────────────────────────────────────────────────── */
static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    /* getPoint() returns the number of current touch points */
    if (s_touch.touched()) {
        GTPoint pt = s_touch.getPoint(0);

        /* Clamp to display area */
        data->point.x = (lv_coord_t)constrain(pt.x, 0, TOUCH_MAX_X - 1);
        data->point.y = (lv_coord_t)constrain(pt.y, 0, TOUCH_MAX_Y - 1);
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/* ──────────────────────────────────────────────────────────────
 * Public API
 * ──────────────────────────────────────────────────────────── */
void touch_init(void)
{
    Wire.begin(TOUCH_SDA_PIN, TOUCH_SCL_PIN);

    /* begin() resets the controller and reads firmware info */
    if (!s_touch.begin(TOUCH_INT_PIN, TOUCH_RST_PIN, &Wire)) {
        Serial.println("[TOUCH] GT911 not found — touch disabled");
        return;
    }

    Serial.printf("[TOUCH] GT911 ready, resolution %d×%d\n",
                  s_touch.getSensorX(), s_touch.getSensorY());

    lv_indev_drv_init(&s_indev_drv);
    s_indev_drv.type    = LV_INDEV_TYPE_POINTER;
    s_indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&s_indev_drv);
}
