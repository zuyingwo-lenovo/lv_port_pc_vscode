#include "lvgl/lvgl.h"
#include "ui/ui.h"
#include "agent_pipeline.h"
#include "audio_sim.h"
#include <pthread.h>
#include <unistd.h>
#include <string>
#include <mutex>
#include <iostream>

// --- Global State ---
static agent_state_t current_state = AGENT_STATE_WAITING;
static std::string last_user_text = "";
static std::string last_agent_text = "";
static std::mutex bridge_mutex;
static bool bridge_running = false;
static pthread_t bridge_thread;
static bool abort_requested = false;
static bool quit_requested = false;

// --- Implement agent_pipeline.h for the UI to poll ---

extern "C" void agent_request_abort(void) {
    std::lock_guard<std::mutex> lock(bridge_mutex);
    abort_requested = true;
}

extern "C" void agent_request_quit(void) {
    std::lock_guard<std::mutex> lock(bridge_mutex);
    quit_requested = true;
}

extern "C" int agent_poll_abort(void) {
    std::lock_guard<std::mutex> lock(bridge_mutex);
    bool res = abort_requested;
    abort_requested = false;
    return res ? 1 : 0;
}

extern "C" int agent_poll_quit(void) {
    std::lock_guard<std::mutex> lock(bridge_mutex);
    bool res = quit_requested;
    quit_requested = false;
    return res ? 1 : 0;
}

extern "C" agent_state_t agent_get_state(void) {
    std::lock_guard<std::mutex> lock(bridge_mutex);
    return current_state;
}

extern "C" const char* agent_get_last_user_text(void) {
    std::lock_guard<std::mutex> lock(bridge_mutex);
    return last_user_text.c_str();
}

extern "C" const char* agent_get_last_agent_text(void) {
    std::lock_guard<std::mutex> lock(bridge_mutex);
    return last_agent_text.c_str();
}

extern "C" void agent_pipeline_init(const char* tts_device) {
    // No-op for bridge, Python handles initialization
}

extern "C" void agent_pipeline_step(void) {
    // No-op
}

// --- Bridge API for Python ---

extern "C" {

void gui_set_state(int state) {
    std::lock_guard<std::mutex> lock(bridge_mutex);
    current_state = (agent_state_t)state;
}

void gui_set_user_text(const char* text) {
    std::lock_guard<std::mutex> lock(bridge_mutex);
    last_user_text = text ? text : "";
}

void gui_set_agent_text(const char* text) {
    std::lock_guard<std::mutex> lock(bridge_mutex);
    last_agent_text = text ? text : "";
}

void gui_set_amplitude(int amp) {
    audio_sim_set_amplitude((uint8_t)amp);
}

static void* gui_loop_thread(void* arg) {
    std::cout << "[GUI Bridge] Starting LVGL thread..." << std::endl;
    lv_init();
    
    // hal.c: sdl_hal_init(w, h)
    void* sdl_hal_init(int w, int h); 
    sdl_hal_init(1920, 480);
    
    ui_init("none"); // Disable C++ audio capture, Python will drive the wave

    while (bridge_running) {

        uint32_t sleep_time_ms = lv_timer_handler();
        if (sleep_time_ms == LV_NO_TIMER_READY) sleep_time_ms = LV_DEF_REFR_PERIOD;
        usleep(sleep_time_ms * 1000);
    }
    
    std::cout << "[GUI Bridge] LVGL thread stopping..." << std::endl;
    return NULL;
}

void gui_start() {
    if (!bridge_running) {
        bridge_running = true;
        pthread_create(&bridge_thread, NULL, gui_loop_thread, NULL);
    }
}

void gui_stop() {
    if (bridge_running) {
        bridge_running = false;
        pthread_join(bridge_thread, NULL);
    }
}

}
