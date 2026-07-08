#include "sim.h"
#include "gear_calc.h"
#include <math.h>
#include <stdlib.h>

typedef struct { char mode; float dur; float tps; } Seg;   // a=accel b=brake h=hold
static const Seg SCRIPT[] = {
    { 'a', 5.5f, 100 }, { 'b', 2.1f, 0 }, { 'h', 1.6f, 35 },
    { 'a', 3.8f, 100 }, { 'b', 1.4f, 0 }, { 'h', 2.2f, 55 },
    { 'a', 7.5f, 100 }, { 'b', 2.6f, 0 }, { 'h', 1.3f, 25 },
};
static const int NSEG = sizeof(SCRIPT) / sizeof(SCRIPT[0]);

static const float ACCEL_RPM_S[5] = { 3300, 2450, 1750, 1250, 950 };
static int   seg;
static float seg_t, sim_t;
static float abs_until;

static float mph_to_kmh(float m) { return m * 1.609344f; }
static float frand(void) { return (float)rand() / (float)RAND_MAX; }

static float sim_speed_kmh(float rpm, int gear) {
    const GearConfig *c = &GEAR_CONFIG_DEFAULT;
    float wheel_rpm = rpm / (c->final_drive * c->ratios[gear - 1]);
    return wheel_rpm * c->tire_circum_m * 60.0f / 1000.0f;
}
static float rpm_from_kmh(float kmh, int gear) {
    const GearConfig *c = &GEAR_CONFIG_DEFAULT;
    return kmh * 1000.0f / 60.0f / c->tire_circum_m * c->final_drive * c->ratios[gear - 1];
}

void sim_init(VehicleState *s)
{
    s->rpm = 1100; s->gear = 1;
    s->coolant_c = 46; s->oil_temp_c = 38; s->oil_temp_valid = true;
    s->oil_pressure_bar = 2.0f; s->battery_v = 13.8f;
    s->ambient_c = 23.5f;
    s->engine_running = true;
    for (int i = 0; i < 4; i++) {
        s->tire_psi[i] = 30.0f + 0.2f * i;
        s->tire_valid[i] = true;
    }
    seg = 0; seg_t = 0; sim_t = 0; abs_until = 0;
}

void sim_step(VehicleState *s, float dt, uint32_t now_ms)
{
    sim_t += dt; seg_t += dt;
    if (seg_t >= SCRIPT[seg].dur) { seg = (seg + 1) % NSEG; seg_t = 0; }
    const Seg *sg = &SCRIPT[seg];

    s->brake_switch = (sg->mode == 'b');
    s->throttle_pct += (sg->tps - s->throttle_pct) * fminf(1.0f, dt * 12);
    s->pedal_pct = s->throttle_pct;

    if (sg->mode == 'a') {
        s->rpm += ACCEL_RPM_S[s->gear - 1] * dt;
        if (s->rpm >= 6350 && s->gear < 5) {
            float v = sim_speed_kmh(s->rpm, s->gear);
            s->gear++; s->rpm = rpm_from_kmh(v, s->gear);
        }
        if (s->rpm > 6580) s->rpm = 6580;
    } else if (sg->mode == 'b') {
        float v = sim_speed_kmh(s->rpm, s->gear) - mph_to_kmh(22) * dt;
        if (v < 45) v = 45;
        if (s->rpm < 3000 && s->gear > 2) s->gear--;
        s->rpm = rpm_from_kmh(v, s->gear);
        if (v > 90 && frand() < dt * 0.9f)
            abs_until = sim_t + 0.35f + frand() * 0.7f;
    } else {
        s->rpm += 90 * dt * (s->throttle_pct / 50.0f);
        if (s->rpm > 6150) s->rpm = 6150;
    }
    if (s->rpm < 1050) s->rpm = 1050;

    s->speed_kmh = sim_speed_kmh(s->rpm, s->gear);
    for (int i = 0; i < 4; i++) s->wheel_kmh[i] = s->speed_kmh;
    s->abs_active = sim_t < abs_until;

    // Warm-up and slow channels
    s->coolant_c += (95.0f + 2.5f * sinf(sim_t / 19) - s->coolant_c) * fminf(1.0f, dt * 0.05f);
    s->oil_temp_c += (111.0f + 3.0f * sinf(sim_t / 27) - s->oil_temp_c) * fminf(1.0f, dt * 0.04f);
    float p = 1.15f + (s->rpm / 6500.0f) * 3.3f - (s->oil_temp_c - 100.0f) * 0.006f;
    s->oil_pressure_bar += (p - s->oil_pressure_bar) * fminf(1.0f, dt * 8);
    s->battery_v = 13.78f + 0.06f * sinf(sim_t / 7);
    s->ambient_c = 23.5f + 0.6f * sinf(sim_t / 41);

    float build = 4.0f * (1.0f - expf(-sim_t / 90.0f));
    for (int i = 0; i < 4; i++)
        s->tire_psi[i] = 30.0f + 0.2f * i + build * (i < 2 ? 1.08f : 1.0f);

    // Sim data is always "fresh"
    s->last_dme_ms = s->last_abs_ms = s->last_cluster_ms = s->last_tpms_ms = now_ms;
}
