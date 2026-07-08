// LVGL UI for the E46 race dash — layout is a 1:1 translation of
// prototype/dash-alt.html at 800x480 (1cqw = 8px).
#pragma once
#include "lvgl.h"
#include "../vehicle_state.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_create(void);   // build the widget tree on the active screen

// Push current data into the widgets. faults = faults_eval() bits;
// now_ms drives blink phases and banner cycling.
void ui_set(const VehicleState *s, uint32_t faults, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
