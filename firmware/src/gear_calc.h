// Calculated gear: no sensor on a manual box, so match engine rpm against
// what each gear predicts from rear wheel speed. Pure logic, unit-testable.
#pragma once
#include "vehicle_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float ratios[5];        // gearbox ratios, 1st..5th
    float final_drive;
    float tire_circum_m;    // rolling circumference
    float tolerance;        // relative rpm mismatch accepted (e.g. 0.10)
} GearConfig;

// E46 330i defaults (Getrag S5D-320Z + 3.38 typical) — set per car.
extern const GearConfig GEAR_CONFIG_DEFAULT;

// Returns 1..5, or 0 for neutral/unknown (too slow, clutch in, or no match).
int gear_calc(const GearConfig *cfg, float rpm, float speed_kmh, bool clutch);

#ifdef __cplusplus
}
#endif
