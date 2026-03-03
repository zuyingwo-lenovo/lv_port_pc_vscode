#include "ui.h"

lv_obj_t * ui_Screen_Main;
#include "components/wave.h"
#include "../audio_sim.h"

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

    lv_obj_t * time_label = lv_label_create(status_bar);
    lv_label_set_text(time_label, "10:42 AM");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x9aa0a6), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_24, 0);
    lv_obj_align(time_label, LV_ALIGN_LEFT_MID, 40, 0);
    
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
