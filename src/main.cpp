/**
 * @file main.cpp
 * ESP32-S3 Temperature Display — Arduino entry point.
 *
 * Wiring summary
 * ──────────────
 *   LCD (SPI2 / HSPI)          Touch (I²C)
 *   SCLK  → GPIO12             SDA  → GPIO6
 *   MOSI  → GPIO11             SCL  → GPIO7
 *   CS    → GPIO10             INT  → GPIO5
 *   DC    → GPIO9              RST  → GPIO4
 *   RST   → GPIO8
 *   BL    → GPIO46
 *
 * Sensor (example — replace with your actual sensor library):
 *   DHT22 DATA → GPIO2  (or any free GPIO)
 *
 * Initialization order matters
 * ─────────────────────────────
 * 1. lv_init()       — must come before display/input driver registration
 * 2. display_init()  — registers the LVGL display driver
 * 3. touch_init()    — registers the LVGL input device
 * 4. ui_init()       — creates LVGL widgets (needs display registered)
 *
 * The LVGL tick source is configured via lv_conf.h (LV_TICK_CUSTOM=1,
 * using millis()), so no separate tick timer is needed.
 */

#include <Arduino.h>
#include <lvgl.h>

#include "display.h"
#include "touch.h"
#include "ui.h"

/* ── Sensor update interval ──────────────────────────────────── */
static const uint32_t SENSOR_UPDATE_MS = 2000;
static uint32_t       s_last_sensor_ms = 0;

/* ──────────────────────────────────────────────────────────────
 * Mock sensor reading — replace with your real sensor driver.
 * E.g. for a DHT22:
 *   #include <DHT.h>
 *   DHT dht(2, DHT22);
 *   float temperature = dht.readTemperature();
 *   float humidity    = dht.readHumidity();
 * ──────────────────────────────────────────────────────────── */
static float read_temperature(void)
{
    /* Simulated 20 – 30 °C sine wave for demonstration */
    return 25.0f + 5.0f * sinf((float)millis() / 10000.0f);
}

static float read_humidity(void)
{
    /* Simulated 50 – 70 % sine wave for demonstration */
    return 60.0f + 10.0f * cosf((float)millis() / 12000.0f);
}

/* ──────────────────────────────────────────────────────────────
 * setup()
 * ──────────────────────────────────────────────────────────── */
void setup(void)
{
    Serial.begin(115200);
    Serial.println("[MAIN] ESP32 Temperature Monitor starting");

    /* 1. Initialise LVGL before registering any drivers */
    lv_init();

    /* 2. Initialise display and register LVGL display driver */
    display_init();

    /* 3. Initialise touch and register LVGL input device */
    touch_init();

    /* 4. Build the temperature UI */
    ui_init();

    Serial.println("[MAIN] Ready");
}

/* ──────────────────────────────────────────────────────────────
 * loop()
 * ──────────────────────────────────────────────────────────── */
void loop(void)
{
    /* Drive LVGL timers (handles animation, refresh, input, etc.) */
    lv_timer_handler();

    /* Read and display sensor values at SENSOR_UPDATE_MS interval */
    uint32_t now = millis();
    if (now - s_last_sensor_ms >= SENSOR_UPDATE_MS) {
        s_last_sensor_ms = now;
        ui_update_sensor(read_temperature(), read_humidity());
    }

    delay(5);  /* yield to FreeRTOS / watchdog */
}
