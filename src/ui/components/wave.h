#ifndef UI_WAVE_H
#define UI_WAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/**
 * Create a wave visualization component
 * @param parent the parent object
 * @return the created object
 */
lv_obj_t * ui_wave_create(lv_obj_t * parent);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // UI_WAVE_H
