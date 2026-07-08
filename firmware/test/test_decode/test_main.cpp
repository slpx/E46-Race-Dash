// Unit tests for the pure-logic modules (decode, gear, faults).
// Run on a PC: pio test -e native   (needs local gcc/clang)
#include <unity.h>
#include <string.h>
#include "vehicle_state.h"
#include "can_ms43.h"
#include "gear_calc.h"
#include "faults.h"

static VehicleState s;

void setUp(void) { memset(&s, 0, sizeof s); }
void tearDown(void) {}

// ── 0x316 DME1: rpm ──────────────────────────────────────────────────────
static void test_dme1_rpm(void)
{
    // 6400 rpm -> raw = 6400*6.4 = 40960 = 0xA000 (LE: B2=0x00, B3=0xA0)
    uint8_t d[8] = { 0, 0, 0x00, 0xA0, 0, 0, 0, 0 };
    TEST_ASSERT_TRUE(ms43_decode(0x316, d, 8, &s, 1000));
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 6400.0f, s.rpm);
    TEST_ASSERT_EQUAL_UINT32(1000, s.last_dme_ms);
}

// ── 0x329 DME2: coolant, throttle, switches ──────────────────────────────
static void test_dme2_coolant_and_switches(void)
{
    // 90 C -> raw = (90+48.373)/0.75 = 184.5 -> 184 gives 89.6 C
    uint8_t d[8] = { 0, 184, 0, 0x11, 127, 254, 0x01, 0 };
    TEST_ASSERT_TRUE(ms43_decode(0x329, d, 8, &s, 0));
    TEST_ASSERT_FLOAT_WITHIN(0.8f, 90.0f, s.coolant_c);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 100.0f, s.throttle_pct);  // 254 = full
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 50.0f, s.pedal_pct);      // 127 ~ half
    TEST_ASSERT_TRUE(s.clutch_switch);
    TEST_ASSERT_TRUE(s.engine_running);
    TEST_ASSERT_TRUE(s.brake_switch);
}

// ── 0x545 DME4: oil temp, lamps ──────────────────────────────────────────
static void test_dme4_oil_temp_and_lamps(void)
{
    // 110 C oil -> raw = 110+48.373 = 158
    uint8_t d[8] = { 0x12, 0x34, 0x12, 0x08, 158, 0x11, 0, 0 };
    TEST_ASSERT_TRUE(ms43_decode(0x545, d, 8, &s, 0));
    TEST_ASSERT_TRUE(s.oil_temp_valid);
    TEST_ASSERT_FLOAT_WITHIN(0.7f, 110.0f, s.oil_temp_c);
    TEST_ASSERT_TRUE(s.lamp_cel);        // 0x12 has bit1
    TEST_ASSERT_TRUE(s.lamp_eml);        // 0x12 has bit4
    TEST_ASSERT_TRUE(s.lamp_overheat);   // 0x08 bit3
    TEST_ASSERT_TRUE(s.lamp_charge);     // 0x11 bit0
    TEST_ASSERT_TRUE(s.lamp_oilp_switch);// 0x11 bit4
    TEST_ASSERT_EQUAL_UINT16(0x1234, s.fuel_counter);
}

static void test_dme4_oil_temp_invalid_when_ff(void)
{
    uint8_t d[8] = { 0, 0, 0, 0, 0xFF, 0, 0, 0 };
    ms43_decode(0x545, d, 8, &s, 0);
    TEST_ASSERT_FALSE(s.oil_temp_valid);
}

// ── 0x1F0 wheel speeds ───────────────────────────────────────────────────
static void test_wheel_speeds(void)
{
    // 100 km/h -> raw = 1600 = 0x640 (12-bit, LE per wheel)
    uint8_t d[8] = { 0x40, 0x06, 0x40, 0x06, 0x40, 0x06, 0x40, 0x06 };
    TEST_ASSERT_TRUE(ms43_decode(0x1F0, d, 8, &s, 0));
    for (int i = 0; i < 4; i++)
        TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, s.wheel_kmh[i]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, s.speed_kmh);
}

// ── 0x1F5 steering ───────────────────────────────────────────────────────
static void test_steering_sign(void)
{
    // +90 deg -> raw = 2000 = 0x07D0
    uint8_t right[8] = { 0xD0, 0x07, 0, 0, 0, 0, 0, 0 };
    ms43_decode(0x1F5, right, 8, &s, 0);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 90.0f, s.steer_deg);
    // Sign bit set -> negative
    uint8_t left[8] = { 0xD0, 0x87, 0, 0, 0, 0, 0, 0 };
    ms43_decode(0x1F5, left, 8, &s, 0);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -90.0f, s.steer_deg);
}

// ── Unknown / short frames ───────────────────────────────────────────────
static void test_rejects_unknown_and_short(void)
{
    uint8_t d[8] = { 0 };
    TEST_ASSERT_FALSE(ms43_decode(0x7FF, d, 8, &s, 0));
    TEST_ASSERT_FALSE(ms43_decode(0x316, d, 4, &s, 0));
}

// ── Gear calc ────────────────────────────────────────────────────────────
static void test_gear_calc_matches_each_gear(void)
{
    const GearConfig *c = &GEAR_CONFIG_DEFAULT;
    for (int g = 1; g <= 5; g++) {
        float kmh = 60.0f;
        float wheel_rpm = kmh * 1000.0f / 60.0f / c->tire_circum_m;
        float rpm = wheel_rpm * c->final_drive * c->ratios[g - 1];
        TEST_ASSERT_EQUAL_INT(g, gear_calc(c, rpm, kmh, false));
    }
}

static void test_gear_calc_neutral_cases(void)
{
    const GearConfig *c = &GEAR_CONFIG_DEFAULT;
    TEST_ASSERT_EQUAL_INT(0, gear_calc(c, 3000, 60, true));   // clutch in
    TEST_ASSERT_EQUAL_INT(0, gear_calc(c, 3000, 2, false));   // stationary
    TEST_ASSERT_EQUAL_INT(0, gear_calc(c, 5000, 200, false)); // no ratio fits
}

// ── Faults ───────────────────────────────────────────────────────────────
static void test_faults_and_banner_cycle(void)
{
    s.last_dme_ms = s.last_abs_ms = 10000;   // fresh
    s.battery_v = 13.8f; s.oil_pressure_bar = 4.0f;
    TEST_ASSERT_EQUAL_UINT32(0, faults_eval(&s, 10000));

    s.lamp_cel = true;
    s.tire_valid[WHEEL_FR] = true; s.tire_psi[WHEEL_FR] = 20.0f;
    uint32_t f = faults_eval(&s, 10000);
    TEST_ASSERT_TRUE(f & FAULT_CEL);
    TEST_ASSERT_TRUE(f & FAULT_TIRE);

    // Two active faults cycle with time: collect both texts
    char buf[32];
    const char *a = faults_banner_text(f, &s, 0, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(a);
    char first[32]; strcpy(first, a);
    const char *b = faults_banner_text(f, &s, FAULT_CYCLE_MS, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_TRUE(strcmp(first, b) != 0);

    // Tire banner names the corner
    uint32_t only_tire = FAULT_TIRE;
    const char *t = faults_banner_text(only_tire, &s, 0, buf, sizeof buf);
    TEST_ASSERT_EQUAL_STRING("TIRE FR", t);
}

static void test_faults_timeout(void)
{
    s.last_dme_ms = 0; s.last_abs_ms = 0;
    s.battery_v = 13.8f; s.oil_pressure_bar = 4.0f;
    uint32_t f = faults_eval(&s, 1000);   // 1 s since last frames
    TEST_ASSERT_TRUE(f & FAULT_ABS);
    TEST_ASSERT_TRUE(f & FAULT_NO_DME);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_dme1_rpm);
    RUN_TEST(test_dme2_coolant_and_switches);
    RUN_TEST(test_dme4_oil_temp_and_lamps);
    RUN_TEST(test_dme4_oil_temp_invalid_when_ff);
    RUN_TEST(test_wheel_speeds);
    RUN_TEST(test_steering_sign);
    RUN_TEST(test_rejects_unknown_and_short);
    RUN_TEST(test_gear_calc_matches_each_gear);
    RUN_TEST(test_gear_calc_neutral_cases);
    RUN_TEST(test_faults_and_banner_cycle);
    RUN_TEST(test_faults_timeout);
    return UNITY_END();
}
