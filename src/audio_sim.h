#ifndef AUDIO_SIM_H
#define AUDIO_SIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/**
 * Initialize the audio simulator
 */
void audio_sim_init(const char* capture_device);

/**
 * Get the current simulated audio amplitude
 * @return Value between 0 (silent) and 255 (loud)
 */
uint8_t audio_sim_get_amplitude(void);

/**
 * Returns true if the user is currently speaking (VAD active)
 */
int audio_sim_is_listening(void);

/**
 * Poll for a completed speech utterence.
 * If 1 is returned, out_buffer contains size elements of 16kHz Float32 mono.
 * The caller must free(*out_buffer).
 */
int audio_sim_get_speech(float **out_buffer, size_t *out_size);

/**
 * Manually set the amplitude (used by bridge)
 */
void audio_sim_set_amplitude(uint8_t amp);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // AUDIO_SIM_H
