#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

// Screen objects
extern lv_obj_t * ui_Screen_Main;

// Functions
void ui_init(const char* capture_device);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // UI_H
