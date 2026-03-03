#include "audio_sim.h"
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PCM_DEVICE "hw:2,0"
#define SAMPLE_RATE 44100
#define FRAMES 256
#define MULTIPLIER 5.0f

static volatile uint8_t current_amplitude = 0;
static pthread_t audio_thread_id;
static int is_running = 0;

// Convert RMS amplitude to 0-255 range based on dB
static uint8_t rms_to_amplitude(double rms) {
    if (rms <= 0.0) return 0;
    
    // Convert to dB relative to max possible 16-bit value
    double db = 20.0 * log10(rms / 32768.0);
    
    // Define useful range, e.g., -60dB (quiet) to 0dB (loudest)
    double min_db = -50.0;
    
    if (db <= min_db) return 0;
    if (db >= 0.0) return 255;
    
    // Map [-50, 0] to [0, 255] roughly
    double normalized = (db - min_db) / (0.0 - min_db);
    int amp = (int)(normalized * 255.0 * MULTIPLIER);
    if (amp > 255) amp = 255;
    return (uint8_t)amp;
}

static void* audio_capture_thread(void* arg) {
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;
    int pcm;
    int16_t buffer[FRAMES * 1]; // 1 channel
    
    // Open PCM device for recording (capture)
    if ((pcm = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "ERROR: Can't open \"%s\" PCM device. %s\n", PCM_DEVICE, snd_strerror(pcm));
        return NULL;
    }
    
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, 1);
    
    unsigned int rate = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0);
    
    snd_pcm_uframes_t frames = FRAMES;
    snd_pcm_hw_params_set_period_size_near(pcm_handle, params, &frames, 0);
    
    if ((pcm = snd_pcm_hw_params(pcm_handle, params)) < 0) {
        fprintf(stderr, "ERROR: Can't set hardware parameters. %s\n", snd_strerror(pcm));
        return NULL;
    }
    
    // Read loop
    while (is_running) {
        pcm = snd_pcm_readi(pcm_handle, buffer, frames);
        
        if (pcm == -EPIPE) {
            snd_pcm_prepare(pcm_handle);
        } else if (pcm < 0) {
            fprintf(stderr, "ERROR: Can't read from PCM device. %s\n", snd_strerror(pcm));
        } else {
            // Calculate RMS
            double sum_squares = 0.0;
            for (int i = 0; i < pcm; i++) {
                double sample = (double)buffer[i];
                sum_squares += sample * sample;
            }
            double rms = sqrt(sum_squares / pcm);
            
            // Map to 0-255 amplitude
            uint8_t target_amp = rms_to_amplitude(rms);
            
            // Smoothly update current_amplitude
            if (current_amplitude < target_amp) {
                current_amplitude += (target_amp - current_amplitude) / 2 + 1;
            } else if (current_amplitude > target_amp) {
                current_amplitude -= (current_amplitude - target_amp) / 4 + 1;
            }
        }
    }
    
    snd_pcm_close(pcm_handle);
    return NULL;
}

void audio_sim_init(void) {
    if (is_running == 0) {
        is_running = 1;
        if (pthread_create(&audio_thread_id, NULL, audio_capture_thread, NULL) != 0) {
            fprintf(stderr, "ERROR: Failed to create audio capture thread\n");
            is_running = 0;
        }
    }
}

uint8_t audio_sim_get_amplitude(void) {
    return current_amplitude;
}

