/**
 * @file ui.cpp
 * Temperature / humidity display UI — built with LVGL 8.
 *
 * Layout (320 × 240 landscape):
 *
 *  ┌──────────────────────────────────────────┐
 *  │  ESP32 Temperature Monitor               │  ← title bar
 *  ├──────────────────────────────────────────┤
 *  │                                          │
 *  │          🌡  25.4 °C                     │  ← large temp label
 *  │                                          │
 *  │          💧  60.0 %                      │  ← humidity label
 *  │                                          │
 *  └──────────────────────────────────────────┘
 */

#include "ui.h"

#include <Arduino.h>
#include <lvgl.h>

/* ──────────────────────────────────────────────────────────────
 * Module-local state (labels updated by ui_update_sensor)
 * ──────────────────────────────────────────────────────────── */
static lv_obj_t *s_temp_label = NULL;
static lv_obj_t *s_humi_label = NULL;

/* ──────────────────────────────────────────────────────────────
 * Public API
 * ──────────────────────────────────────────────────────────── */
void ui_init(void)
{
    lv_obj_t *scr = lv_scr_act();

    /* ── Background colour ────────────────────────────────────── */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1A1A2E), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    /* ── Title bar ────────────────────────────────────────────── */
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "ESP32 Temperature Monitor");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00D4FF), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* ── Horizontal separator ─────────────────────────────────── */
    lv_obj_t *line = lv_obj_create(scr);
    lv_obj_set_size(line, 300, 2);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 28);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x00D4FF), LV_PART_MAIN);
    lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line, 0, LV_PART_MAIN);

    /* ── Temperature label ────────────────────────────────────── */
    s_temp_label = lv_label_create(scr);
    lv_label_set_text(s_temp_label, "-- °C");
    lv_obj_set_style_text_color(s_temp_label,
                                 lv_color_hex(0xFF6B6B), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_temp_label,
                                &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_align(s_temp_label, LV_ALIGN_CENTER, 0, -20);

    /* ── Humidity label ───────────────────────────────────────── */
    s_humi_label = lv_label_create(scr);
    lv_label_set_text(s_humi_label, "-- %RH");
    lv_obj_set_style_text_color(s_humi_label,
                                 lv_color_hex(0x4ECDC4), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_humi_label,
                                &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(s_humi_label, LV_ALIGN_CENTER, 0, 60);
}

void ui_update_sensor(float temperature, float humidity)
{
    if (!s_temp_label || !s_humi_label) return;

    char buf[32];

    snprintf(buf, sizeof(buf), "%.1f °C", (double)temperature);
    lv_label_set_text(s_temp_label, buf);

    snprintf(buf, sizeof(buf), "%.1f %%RH", (double)humidity);
    lv_label_set_text(s_humi_label, buf);
}
