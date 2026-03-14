#include "ui.h"

lv_obj_t * ui_Screen_Main;
lv_obj_t * time_label;
lv_obj_t * date_label;

#include "components/wave.h"
#include "../audio_sim.h"
#include "../agent_pipeline.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(font_montserrat_72);

static lv_obj_t * status_text;
static lv_obj_t * chat_list;

static char last_displayed_user_text[512] = {0};
static char last_displayed_agent_text[4096] = {0};
static lv_obj_t * live_user_label = NULL;
static lv_obj_t * live_agent_label = NULL;

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
    
    snprintf(time_str, sizeof(time_str), "%02d:%02d", hour, tm_info->tm_min);
    lv_label_set_text(time_label, time_str);

    char date_str[32];
    strftime(date_str, sizeof(date_str), "%a, %b %d  %p", tm_info); 
    lv_label_set_text(date_label, date_str);
}

static lv_obj_t * create_bubble(bool is_user) {
    lv_obj_t * row = lv_obj_create(chat_list);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    
    lv_obj_t * bubble = lv_obj_create(row);
    lv_obj_set_style_bg_opa(bubble, 140, 0);
    lv_obj_set_style_border_width(bubble, 1, 0);
    lv_obj_set_style_radius(bubble, 12, 0);
    lv_obj_set_style_pad_all(bubble, 15, 0);
    lv_obj_set_width(bubble, 340); 
    lv_obj_set_height(bubble, LV_SIZE_CONTENT);

    if (is_user) {
        lv_obj_align(bubble, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_obj_set_style_bg_color(bubble, lv_color_hex(0x0f172a), 0);
        lv_obj_set_style_border_color(bubble, lv_color_hex(0x3b82f6), 0);
        lv_obj_set_style_shadow_color(bubble, lv_color_hex(0x3b82f6), 0);
        lv_obj_set_style_shadow_width(bubble, 10, 0);
        lv_obj_set_style_shadow_opa(bubble, 100, 0);
    } else {
        lv_obj_align(bubble, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_style_bg_color(bubble, lv_color_hex(0x064e3b), 0);
        lv_obj_set_style_border_color(bubble, lv_color_hex(0x10b981), 0);
        lv_obj_set_style_shadow_color(bubble, lv_color_hex(0x10b981), 0);
        lv_obj_set_style_shadow_width(bubble, 10, 0);
        lv_obj_set_style_shadow_opa(bubble, 100, 0);
    }

    lv_obj_t * label = lv_label_create(bubble);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_color(label, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    
    // Add small title
    lv_obj_t * title = lv_label_create(bubble);
    lv_label_set_text(title, is_user ? "User" : "Astra");
    lv_obj_set_style_text_color(title, is_user ? lv_color_hex(0x94a3b8) : lv_color_hex(0x6ee7b7), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, -5);
    
    // Push main label down slightly
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 20);
    
    // Adjust bubble padding bottom
    lv_obj_set_style_pad_bottom(bubble, 40, 0);

    return label;
}

static void agent_status_timer_cb(lv_timer_t * timer)
{
    agent_state_t state = agent_get_state();
    
    switch (state) {
        case AGENT_STATE_WAITING:
            lv_label_set_text(status_text, "TRIAX ACTIVE | WAITING");
            lv_obj_set_style_text_color(status_text, lv_color_hex(0x00e5ff), 0);
            break;
        case AGENT_STATE_LISTENING:
            lv_label_set_text(status_text, "TRIAX ACTIVE | LISTENING");
            lv_obj_set_style_text_color(status_text, lv_color_hex(0xffaa00), 0);
            break;
        case AGENT_STATE_PROCESSING:
            lv_label_set_text(status_text, "TRIAX ACTIVE | PROCESSING");
            lv_obj_set_style_text_color(status_text, lv_color_hex(0xff00ff), 0);
            break;
        case AGENT_STATE_SPEAKING:
            lv_label_set_text(status_text, "TRIAX ACTIVE | SPEAKING");
            lv_obj_set_style_text_color(status_text, lv_color_hex(0x00ff00), 0);
            break;
    }

    const char* user_str = agent_get_last_user_text();
    if (user_str && user_str[0] != '\0') {
        if (strcmp(user_str, last_displayed_user_text) != 0) {
            strncpy(last_displayed_user_text, user_str, sizeof(last_displayed_user_text)-1);
            if (!live_user_label) {
                live_user_label = create_bubble(true);
            }
            lv_label_set_text(live_user_label, user_str);
            lv_obj_scroll_to_view(lv_obj_get_parent(lv_obj_get_parent(live_user_label)), LV_ANIM_OFF);
        }
    } else {
        live_user_label = NULL;
    }

    const char* agent_str = agent_get_last_agent_text();
    if (agent_str && agent_str[0] != '\0') {
        if (strcmp(agent_str, last_displayed_agent_text) != 0) {
            strncpy(last_displayed_agent_text, agent_str, sizeof(last_displayed_agent_text)-1);
            if (!live_agent_label) {
                live_agent_label = create_bubble(false);
            }
            lv_label_set_text(live_agent_label, agent_str);
            lv_obj_scroll_to_view(lv_obj_get_parent(lv_obj_get_parent(live_agent_label)), LV_ANIM_OFF);
        }
    } else {
        live_agent_label = NULL;
    }
}

void ui_init(const char* capture_device)
{
    // Create main screen
    ui_Screen_Main = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen_Main, LV_OBJ_FLAG_SCROLLABLE);

    // Deep dark background for the modern glass UI look
    lv_obj_set_style_bg_color(ui_Screen_Main, lv_color_hex(0x020205), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen_Main, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    // Remove borders
    lv_obj_set_style_border_width(ui_Screen_Main, 0, 0);
    lv_obj_set_style_pad_all(ui_Screen_Main, 0, 0);

    // Initialize audio sim
    audio_sim_init(capture_device);

    // --- LEFT PANEL (Status & Time) ---
    lv_obj_t * left_panel = lv_obj_create(ui_Screen_Main);
    lv_obj_set_size(left_panel, 420, 480);
    lv_obj_align(left_panel, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(left_panel, lv_color_hex(0x0a0f18), 0);
    lv_obj_set_style_bg_opa(left_panel, 180, 0);  // Translucent panel
    lv_obj_set_style_border_width(left_panel, 0, 0);
    lv_obj_set_style_border_color(left_panel, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_radius(left_panel, 0, 0);
    lv_obj_clear_flag(left_panel, LV_OBJ_FLAG_SCROLLABLE);

    // Top icons mock (Wi-Fi, signal, bat)
    lv_obj_t * sys_icons = lv_label_create(left_panel);
    lv_label_set_text(sys_icons, LV_SYMBOL_WIFI "   " LV_SYMBOL_BLUETOOTH "   " LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(sys_icons, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_text_font(sys_icons, &lv_font_montserrat_18, 0);
    lv_obj_align(sys_icons, LV_ALIGN_TOP_LEFT, 20, 20);

    // Time Label
    time_label = lv_label_create(left_panel);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_text_font(time_label, &font_montserrat_72, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -20);
    
    // Date Label
    date_label = lv_label_create(left_panel);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_32, 0);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 30);

    // Status / Mode Label
    status_text = lv_label_create(left_panel);
    lv_label_set_text(status_text, "TRIAX ACTIVE | WAITING");
    lv_obj_set_style_text_color(status_text, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_text_font(status_text, &lv_font_montserrat_16, 0);
    lv_obj_align(status_text, LV_ALIGN_BOTTOM_LEFT, 20, -20);

    // Set initial time and start timer
    update_time_cb(NULL);
    lv_timer_create(update_time_cb, 60000, NULL);
    
    
    // --- CENTER PANEL (Voice Wave) ---
    lv_obj_t * center_panel = lv_obj_create(ui_Screen_Main);
    lv_obj_set_size(center_panel, 1080, 480);
    lv_obj_align(center_panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(center_panel, 0, 0);
    lv_obj_set_style_border_width(center_panel, 0, 0);
    lv_obj_clear_flag(center_panel, LV_OBJ_FLAG_SCROLLABLE);

    // Title inside center
    // lv_obj_t * center_title = lv_label_create(center_panel);
    // lv_label_set_text(center_title, LV_SYMBOL_AUDIO "  Mic: ACTIVE");
    // lv_obj_set_style_text_color(center_title, lv_color_hex(0x00e5ff), 0);
    // lv_obj_set_style_text_font(center_title, &lv_font_montserrat_18, 0);
    // lv_obj_align(center_title, LV_ALIGN_TOP_LEFT, 20, 20);
    
    // Bottom TRIAX branding
    lv_obj_t * astra_brand = lv_label_create(center_panel);
    lv_label_set_text(astra_brand, "T R I A X");
    lv_obj_set_style_text_color(astra_brand, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_text_font(astra_brand, &lv_font_montserrat_24, 0);
    lv_obj_align(astra_brand, LV_ALIGN_BOTTOM_MID, 0, -20);

    // Add Wave Visualization to center panel
    ui_wave_create(center_panel);


    // --- RIGHT PANEL (Conversation History) ---
    lv_obj_t * right_panel = lv_obj_create(ui_Screen_Main);
    lv_obj_set_size(right_panel, 420, 480);
    lv_obj_align(right_panel, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(right_panel, lv_color_hex(0x0a0f18), 0);
    lv_obj_set_style_bg_opa(right_panel, 180, 0);
    lv_obj_set_style_border_width(right_panel, 0, 0);
    lv_obj_set_style_border_color(right_panel, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_radius(right_panel, 0, 0);
    lv_obj_clear_flag(right_panel, LV_OBJ_FLAG_SCROLLABLE); // Parent wrapper non scrollable

    chat_list = lv_obj_create(right_panel);
    lv_obj_set_size(chat_list, 420, 480);
    lv_obj_align(chat_list, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(chat_list, 0, 0);
    lv_obj_set_style_border_width(chat_list, 0, 0);
    lv_obj_set_style_pad_all(chat_list, 15, 0);
    lv_obj_set_flex_flow(chat_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(chat_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Start Agent Status Timer
    lv_timer_create(agent_status_timer_cb, 100, NULL);

    // Apply the screen immediately
    lv_screen_load(ui_Screen_Main);
}
