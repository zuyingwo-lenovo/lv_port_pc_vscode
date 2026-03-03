#include "ui.h"

lv_obj_t * ui_Screen_Main;
lv_obj_t * time_label;

#include "components/wave.h"
#include "../audio_sim.h"
#include <time.h>
#include <stdio.h>

static void update_time_cb(lv_timer_t * timer)
{
    time_t t = time(NULL);
    // Force JST (+9 hours from UTC)
    t += (9 * 60 * 60);
    struct tm * tm_info = gmtime(&t);
    
    char time_str[16];
    int hour = tm_info->tm_hour;
    int ampm = 0; // 0 = AM, 1 = PM
    
    if (hour >= 12) {
        ampm = 1;
        if (hour > 12) hour -= 12;
    }
    if (hour == 0) hour = 12;
    
    snprintf(time_str, sizeof(time_str), "%d:%02d %s", 
             hour, 
             tm_info->tm_min, 
             ampm ? "PM" : "AM");
             
    lv_label_set_text(time_label, time_str);
}

void ui_init(void)
{
    // Create main screen
    ui_Screen_Main = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen_Main, LV_OBJ_FLAG_SCROLLABLE);

    // Deep dark background for the modern glass UI look
    lv_obj_set_style_bg_color(ui_Screen_Main, lv_color_hex(0x020205), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen_Main, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Initialize audio sim
    audio_sim_init();

    // Add Top Status Bar container
    lv_obj_t * status_bar = lv_obj_create(ui_Screen_Main);
    lv_obj_set_width(status_bar, 1920);
    lv_obj_set_height(status_bar, 60);
    lv_obj_set_style_bg_opa(status_bar, 0, 0); // Transparent
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 10);

    time_label = lv_label_create(status_bar);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x9aa0a6), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_24, 0);
    lv_obj_align(time_label, LV_ALIGN_LEFT_MID, 40, 0);
    
    // Set initial time and start timer
    update_time_cb(NULL);
    lv_timer_create(update_time_cb, 60000, NULL);
    
    // Wi-Fi / Status icons mock
    lv_obj_t * icons_label = lv_label_create(status_bar);
    lv_label_set_text(icons_label, LV_SYMBOL_WIFI " " LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(icons_label, lv_color_hex(0x9aa0a6), 0);
    lv_obj_set_style_text_font(icons_label, &lv_font_montserrat_24, 0);
    lv_obj_align(icons_label, LV_ALIGN_RIGHT_MID, -40, 0);

    // Pulse Text Status Indicator
    lv_obj_t * status_text = lv_label_create(ui_Screen_Main);
    lv_label_set_text(status_text, "AWAITING WAKE WORD...");
    lv_obj_set_style_text_color(status_text, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_text_font(status_text, &lv_font_montserrat_24, 0);
    // Align directly above the wave canvas
    lv_obj_align(status_text, LV_ALIGN_CENTER, 0, -150);

    // Add Wave Visualization
    ui_wave_create(ui_Screen_Main);

    // Apply the screen immediately
    lv_screen_load(ui_Screen_Main);
}
