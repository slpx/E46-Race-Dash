// Central vehicle state — everything the dash renders, in SI units.
// CAN decode / ADC / TPMS write into this; UI reads and converts to
// display units (mph, °F, psi) at render time.
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    // From MS43 DME
    float rpm;                // 0x316
    float coolant_c;          // 0x329
    float oil_temp_c;         // 0x545 (0xFF raw = sensor absent -> oil_temp_valid false)
    bool  oil_temp_valid;
    float throttle_pct;       // 0x329 actual throttle plate
    float pedal_pct;          // 0x329 driver demand
    bool  brake_switch;       // 0x329
    bool  clutch_switch;      // 0x329
    bool  engine_running;     // 0x329
    uint16_t fuel_counter;    // 0x545 rolling consumption counter (raw)
    bool  lamp_cel;           // 0x545
    bool  lamp_eml;           // 0x545
    bool  lamp_charge;        // 0x545
    bool  lamp_overheat;      // 0x545
    bool  lamp_oilp_switch;   // 0x545 (idiot-light bit only, not a pressure)

    // From ABS/DSC (MK60)
    float wheel_kmh[4];       // 0x1F0: FL FR RL RR
    float steer_deg;          // 0x1F5: +right / -left
    bool  abs_active;         // intervention (bits TBD from capture -> see can_ms43.cpp)

    // From instrument cluster (IKE)
    uint8_t fuel_level_raw;   // 0x613 B2 (decode to gallons later; sender curve TBD)
    float ambient_c;          // 0x615

    // From analog inputs (ADS1115) — not CAN
    float oil_pressure_bar;
    float battery_v;
    bool  turn_left;
    bool  turn_right;

    // From TPMS (2nd CAN bus) — frame layout TBD by chosen unit
    float tire_psi[4];        // FL FR RL RR
    bool  tire_valid[4];

    // Derived
    int   gear;               // 0 = N/unknown, 1..5
    float speed_kmh;          // driven from rear wheel average

    // Freshness (millis of last frame per source) for timeout faults
    uint32_t last_dme_ms;
    uint32_t last_abs_ms;
    uint32_t last_cluster_ms;
    uint32_t last_tpms_ms;
} VehicleState;

enum { WHEEL_FL = 0, WHEEL_FR = 1, WHEEL_RL = 2, WHEEL_RR = 3 };
