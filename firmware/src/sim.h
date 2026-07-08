// Bench simulator: drives VehicleState through scripted laps so the UI can
// be developed with no car attached (SIM_MODE=1 in platformio.ini).
// Mirrors the HTML prototype's sim: accel / brake / corner segments,
// warm-up from cold, ABS flutter under braking.
#pragma once
#include "vehicle_state.h"

#ifdef __cplusplus
extern "C" {
#endif

void sim_init(VehicleState *s);
void sim_step(VehicleState *s, float dt_s, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
