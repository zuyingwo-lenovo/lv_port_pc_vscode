#ifndef AUDIO_SIM_H
#define AUDIO_SIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Initialize the audio simulator
 */
void audio_sim_init(void);

/**
 * Get the current simulated audio amplitude
 * @return Value between 0 (silent) and 255 (loud)
 */
uint8_t audio_sim_get_amplitude(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // AUDIO_SIM_H
