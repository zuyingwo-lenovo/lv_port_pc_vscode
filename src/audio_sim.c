#include "audio_sim.h"
#include "lvgl/lvgl.h"
#include <stdlib.h>

static uint8_t current_amplitude = 0;
static uint8_t target_amplitude = 0;
static bool is_listening = false;
static uint32_t state_timer = 0;

static void audio_sim_timer_cb(lv_timer_t * timer)
{
    uint32_t now = lv_tick_get();

    // State machine: alternate between "silent" and "talking" every few seconds
    if (now - state_timer > 4000) {
        state_timer = now;
        is_listening = !is_listening;
    }

    if (is_listening) {
        // Generate random amplitude when "talking"
        target_amplitude = 50 + (rand() % 150); // between 50 and 200
    } else {
        target_amplitude = 10; // Idle noise floor
    }

    // Smooth transition to target amplitude
    if (current_amplitude < target_amplitude) {
        current_amplitude += (target_amplitude - current_amplitude) / 4 + 1;
    } else if (current_amplitude > target_amplitude) {
        current_amplitude -= (current_amplitude - target_amplitude) / 4 + 1;
    }
}

void audio_sim_init(void)
{
    state_timer = lv_tick_get();
    lv_timer_create(audio_sim_timer_cb, 50, NULL); // Update audio 20 times per second
}

uint8_t audio_sim_get_amplitude(void)
{
    return current_amplitude;
}
