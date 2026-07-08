#include "gear_calc.h"
#include <math.h>

const GearConfig GEAR_CONFIG_DEFAULT = {
    .ratios = { 4.23f, 2.52f, 1.66f, 1.22f, 1.00f },
    .final_drive = 3.38f,
    .tire_circum_m = 1.99f,
    .tolerance = 0.10f,
};

int gear_calc(const GearConfig *cfg, float rpm, float speed_kmh, bool clutch)
{
    if (clutch || speed_kmh < 5.0f || rpm < 800.0f) return 0;

    // Wheel rpm from road speed
    const float wheel_rpm = speed_kmh * (1000.0f / 60.0f) / cfg->tire_circum_m;

    int best = 0;
    float best_err = cfg->tolerance;
    for (int g = 0; g < 5; g++) {
        const float expected = wheel_rpm * cfg->final_drive * cfg->ratios[g];
        const float err = fabsf(rpm - expected) / expected;
        if (err < best_err) { best_err = err; best = g + 1; }
    }
    return best;
}
