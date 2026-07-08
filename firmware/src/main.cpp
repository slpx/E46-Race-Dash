// E46 Race Dash — firmware entry point.
//
// Milestone 1 (this code): CAN decode pipeline + bench sim, state printed
// over serial at 10 Hz. Proves the bus wiring and decode before any UI work.
// Milestone 2: LVGL bring-up on the 800x480 RGB panel (layout per
// prototype/dash-alt.html). Milestone 3: ADS1115 + MCP2515/TPMS.

#include <Arduino.h>
#include "vehicle_state.h"
#include "can_ms43.h"
#include "gear_calc.h"
#include "faults.h"
#include "sim.h"
#include "board_pins.h"

#if !SIM_MODE
#include "driver/twai.h"
#endif

static VehicleState g_state;
static portMUX_TYPE g_state_mux = portMUX_INITIALIZER_UNLOCKED;

// ── Live CAN (listen-only) ───────────────────────────────────────────────
#if !SIM_MODE
static void can_task(void *arg)
{
    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)PIN_CAN_TX, (gpio_num_t)PIN_CAN_RX, TWAI_MODE_LISTEN_ONLY);
    g.rx_queue_len = 32;
    twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();  // filter in sw

    if (twai_driver_install(&g, &t, &f) != ESP_OK || twai_start() != ESP_OK) {
        Serial.println("FATAL: TWAI init failed");
        vTaskDelete(NULL);
    }

    twai_message_t msg;
    for (;;) {
        if (twai_receive(&msg, pdMS_TO_TICKS(100)) == ESP_OK && !msg.rtr) {
            taskENTER_CRITICAL(&g_state_mux);
            ms43_decode(msg.identifier, msg.data, msg.data_length_code,
                        &g_state, millis());
            taskEXIT_CRITICAL(&g_state_mux);
        }
    }
}
#endif

// ── Setup / loop ─────────────────────────────────────────────────────────
void setup()
{
    Serial.begin(115200);
    Serial.println("\nE46 Race Dash — fw milestone 1 ("
#if SIM_MODE
        "SIM"
#else
        "LIVE CAN, listen-only"
#endif
        ")");

#if SIM_MODE
    sim_init(&g_state);
#else
    xTaskCreatePinnedToCore(can_task, "can", 4096, NULL, 10, NULL, 0);
#endif
}

void loop()
{
    static uint32_t last = 0;
    const uint32_t now = millis();
    const float dt = (now - last) / 1000.0f;
    last = now;

#if SIM_MODE
    sim_step(&g_state, dt, now);
#endif

    // Snapshot state under lock, then work on the copy
    taskENTER_CRITICAL(&g_state_mux);
    VehicleState s = g_state;
    taskEXIT_CRITICAL(&g_state_mux);

    s.gear = gear_calc(&GEAR_CONFIG_DEFAULT, s.rpm, s.speed_kmh, s.clutch_switch);
    const uint32_t faults = faults_eval(&s, now);

    // 10 Hz status line (imperial, matching the dash display)
    static uint32_t last_print = 0;
    if (now - last_print >= 100) {
        last_print = now;
        char banner[32];
        const char *b = faults_banner_text(faults, &s, now, banner, sizeof banner);
        Serial.printf(
            "rpm=%4.0f gear=%d mph=%3.0f wtr=%3.0fF oilt=%3.0fF oilp=%2.0fpsi "
            "bat=%4.1fV tps=%3.0f%% brk=%d abs=%d tire=%4.1f/%4.1f/%4.1f/%4.1f %s\n",
            s.rpm, s.gear, s.speed_kmh * 0.621371f,
            s.coolant_c * 1.8f + 32.0f,
            s.oil_temp_valid ? s.oil_temp_c * 1.8f + 32.0f : -1.0f,
            s.oil_pressure_bar * 14.5038f,
            s.battery_v, s.throttle_pct, s.brake_switch, s.abs_active,
            s.tire_psi[0], s.tire_psi[1], s.tire_psi[2], s.tire_psi[3],
            b ? b : "");
    }

    delay(10);
}
