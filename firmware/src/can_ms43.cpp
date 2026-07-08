#include "can_ms43.h"

static inline uint16_t le16(const uint8_t *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

bool ms43_decode(uint32_t id, const uint8_t *d, uint8_t len,
                 VehicleState *s, uint32_t now_ms)
{
    if (len < 8) return false;   // all frames we care about are 8 bytes

    switch (id) {

    case CANID_DME1:  // ~10 ms
        s->rpm = le16(&d[2]) / 6.4f;
        s->last_dme_ms = now_ms;
        return true;

    case CANID_DME2:  // ~10 ms
        s->coolant_c      = 0.75f * d[1] - 48.373f;
        s->pedal_pct      = d[4] * (100.0f / 254.0f);
        s->throttle_pct   = d[5] * (100.0f / 254.0f);
        s->clutch_switch  = d[3] & 0x01;
        s->engine_running = d[3] & 0x10;
        s->brake_switch   = d[6] & 0x01;
        s->last_dme_ms = now_ms;
        return true;

    case CANID_DME4: {  // ~10 ms
        s->lamp_cel      = d[0] & 0x02;
        s->lamp_eml      = d[0] & 0x10;
        s->fuel_counter  = le16(&d[1]);
        s->lamp_overheat = d[3] & 0x08;
        // Oil temp from the oil condition sensor; 0xFF = sensor absent/invalid
        s->oil_temp_valid = (d[4] != 0xFF);
        if (s->oil_temp_valid) s->oil_temp_c = d[4] - 48.373f;
        s->lamp_charge      = d[5] & 0x01;
        s->lamp_oilp_switch = d[5] & 0x10;
        s->last_dme_ms = now_ms;
        return true;
    }

    case CANID_ASC1:  // ~10 ms
        // TODO(capture): exact MK60 intervention bit positions need a live
        // dump. Placeholder: byte0 low bits carry ASC/ABS-in-regulation flags.
        s->abs_active = d[0] & 0x02;
        s->last_abs_ms = now_ms;
        return true;

    case CANID_WHEELS: {  // ~10 ms, 4x 12-bit LE, 0.0625 km/h per bit
        for (int i = 0; i < 4; i++) {
            uint16_t raw = le16(&d[i * 2]) & 0x0FFF;  // upper bits: verify in capture
            s->wheel_kmh[i] = raw * 0.0625f;
        }
        // Rear axle average = vehicle speed (RWD; fronts read low under lockup)
        s->speed_kmh = 0.5f * (s->wheel_kmh[WHEEL_RL] + s->wheel_kmh[WHEEL_RR]);
        s->last_abs_ms = now_ms;
        return true;
    }

    case CANID_STEER: {  // ~10 ms, 15-bit + sign, 0.045 deg/bit
        uint16_t raw = le16(&d[0]);
        float deg = (raw & 0x7FFF) * 0.045f;
        s->steer_deg = (raw & 0x8000) ? -deg : deg;
        s->last_abs_ms = now_ms;
        return true;
    }

    case CANID_CLUSTER:  // ~200 ms, needs IKE alive
        s->fuel_level_raw = d[2];
        s->last_cluster_ms = now_ms;
        return true;

    case CANID_IKE_AMB:  // ~200 ms
        // TODO(capture): confirm offset/sign on this car (raw °C assumed)
        s->ambient_c = (float)(int8_t)d[3];
        s->last_cluster_ms = now_ms;
        return true;

    default:
        return false;
    }
}
