#include "wave.h"
#include "../../audio_sim.h"
#include <math.h>

#define CANVAS_WIDTH 1080
#define CANVAS_HEIGHT 440
#define NUM_WAVES 6

typedef struct {
    lv_color_t color;
    float freq;
    float phase;
    float speed;
    float base_amp;
    lv_point_precise_t * points;
} wave_layer_t;

static wave_layer_t layers[NUM_WAVES];
static lv_draw_line_dsc_t line_dsc[NUM_WAVES];
static lv_obj_t * wave_canvas;
static uint8_t * cbuf;

static void wave_anim_cb(void * var, int32_t v)
{
    // This function is no longer used by lv_anim_t but kept for structure if needed
}

static void wave_update_timer_cb(lv_timer_t * timer)
{
    lv_obj_t * canvas = (lv_obj_t *)lv_timer_get_user_data(timer);
    
    // Get live audio amplitude
    uint8_t current_amp = audio_sim_get_amplitude();
    
    // Threshold to decide whether to draw or clear
    const uint8_t AMPLITUDE_THRESHOLD = 5; 
    
    static bool was_drawing = false;

    if (current_amp > AMPLITUDE_THRESHOLD) {
        // We are "talking", so draw the waves
        
        // Clear canvas for new frame
        lv_canvas_fill_bg(canvas, lv_color_hex(0x020205), LV_OPA_0);

        float amp_multiplier = 1.0f + (current_amp / 50.0f); // Scales visuals based on volume
        
        // Draw each wave
        for(int i = 0; i < NUM_WAVES; i++) {
            layers[i].phase += layers[i].speed * (1.0f + (current_amp / 100.0f)); // Spin faster when loud
            
            for(int x = 0; x < CANVAS_WIDTH; x += 10) { // Step size of 10 for performance
                float normalized_x = (float)x / CANVAS_WIDTH;
                // Window function (sine curve) to taper edges towards 0
                float window = sinf(normalized_x * 3.14159f); 
                
                float y = sinf(x * layers[i].freq + layers[i].phase) * 
                          layers[i].base_amp * amp_multiplier * window;
                
                layers[i].points[x/10].x = x;
                layers[i].points[x/10].y = (CANVAS_HEIGHT / 2) + (int)y;
            }
            
            lv_layer_t layer;
            lv_canvas_init_layer(canvas, &layer);
            
            line_dsc[i].points = layers[i].points;
            line_dsc[i].point_cnt = CANVAS_WIDTH / 10;
            lv_draw_line(&layer, &line_dsc[i]);
            lv_canvas_finish_layer(canvas, &layer);
        }
        
        was_drawing = true;
        
    } else {
        // Silent frame
        if (was_drawing) {
            // Clear the canvas once when transitioning to silence to remove lingering waves
            lv_canvas_fill_bg(canvas, lv_color_hex(0x020205), LV_OPA_0);
            was_drawing = false;
        }
        // If not drawing, we do nothing. The canvas remains transparent/clear.
        // This saves enormous amount of CPU!
    }
}

lv_obj_t * ui_wave_create(lv_obj_t * parent)
{
    // Create drawing buffer for the canvas
    cbuf = lv_malloc(LV_CANVAS_BUF_SIZE(CANVAS_WIDTH, CANVAS_HEIGHT, 32, LV_DRAW_BUF_STRIDE_ALIGN));
    
    wave_canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(wave_canvas, cbuf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_ARGB8888);
    lv_obj_center(wave_canvas);

    // Initialize layers
    lv_color_t colors[] = { lv_color_hex(0x00e5ff), lv_color_hex(0xbc00dd), lv_color_hex(0xff00ff), lv_color_white() };
    float freqs[] = { 0.015f, 0.025f, 0.010f, 0.035f };
    float amps[] = { 40.0f, 30.0f, 50.0f, 20.0f };
    float speeds[] = { 0.04f, 0.05f, 0.03f, 0.06f };
    
    for(int i = 0; i < NUM_WAVES; i++) {
        layers[i].color = colors[i];
        layers[i].freq = freqs[i];
        layers[i].phase = i * 2.0f;
        layers[i].speed = speeds[i];
        layers[i].base_amp = amps[i];
        
        layers[i].points = lv_malloc(sizeof(lv_point_precise_t) * (CANVAS_WIDTH / 10 + 1));
        
        lv_draw_line_dsc_init(&line_dsc[i]);
        line_dsc[i].color = layers[i].color;
        line_dsc[i].width = 3;
        line_dsc[i].round_start = 1;
        line_dsc[i].round_end = 1;
    }

    // Replace infinite lv_anim_t with a periodic timer running at ~30 FPS (33ms)
    // The timer logic will decide whether rendering is needed based on DB threshold.
    lv_timer_create(wave_update_timer_cb, 33, wave_canvas);
    
    return wave_canvas;
}
