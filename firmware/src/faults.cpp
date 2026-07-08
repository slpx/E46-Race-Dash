#include "faults.h"
#include <stdio.h>
#include <string.h>

static const float TIRE_LOW_PSI = 26.0f;

uint32_t faults_eval(const VehicleState *s, uint32_t now_ms)
{
    uint32_t f = 0;

    if (s->oil_pressure_bar < 1.4f && s->rpm > 3000.0f) f |= FAULT_OIL_PRESSURE;
    if (s->coolant_c >= 106.0f || s->lamp_overheat)     f |= FAULT_OVERHEAT;
    if (s->lamp_cel)                                    f |= FAULT_CEL;
    if (s->lamp_eml)                                    f |= FAULT_EML;
    if (s->lamp_charge || s->battery_v < 12.6f)         f |= FAULT_CHARGE;

    if (now_ms - s->last_abs_ms > SRC_TIMEOUT_MS)       f |= FAULT_ABS;
    if (now_ms - s->last_dme_ms > SRC_TIMEOUT_MS)       f |= FAULT_NO_DME;

    for (int i = 0; i < 4; i++)
        if (s->tire_valid[i] && s->tire_psi[i] < TIRE_LOW_PSI) f |= FAULT_TIRE;

    return f;
}

static int tire_text(const VehicleState *s, char *buf, int buflen)
{
    static const char *corner[4] = { "FL", "FR", "RL", "RR" };
    int n = snprintf(buf, buflen, "TIRE");
    for (int i = 0; i < 4; i++)
        if (s->tire_valid[i] && s->tire_psi[i] < TIRE_LOW_PSI)
            n += snprintf(buf + n, buflen - n, " %s", corner[i]);
    return n;
}

const char *faults_banner_text(uint32_t faults, const VehicleState *s,
                               uint32_t now_ms, char *buf, int buflen)
{
    if (!faults) return NULL;

    // Stable priority order (most critical first)
    struct { uint32_t bit; const char *text; } table[] = {
        { FAULT_OIL_PRESSURE, "OIL PRESSURE" },
        { FAULT_OVERHEAT,     "OVERHEAT" },
        { FAULT_NO_DME,       "NO DME DATA" },
        { FAULT_ABS,          "ABS FAULT" },
        { FAULT_TIRE,         NULL },          // built dynamically
        { FAULT_CHARGE,       "CHARGE" },
        { FAULT_CEL,          "CHECK ENGINE" },
        { FAULT_EML,          "EML" },
    };
    const int ntable = sizeof(table) / sizeof(table[0]);

    int active = 0;
    for (int i = 0; i < ntable; i++)
        if (faults & table[i].bit) active++;

    int pick = (now_ms / FAULT_CYCLE_MS) % active;

    for (int i = 0; i < ntable; i++) {
        if (!(faults & table[i].bit)) continue;
        if (pick-- == 0) {
            if (table[i].bit == FAULT_TIRE) { tire_text(s, buf, buflen); return buf; }
            snprintf(buf, buflen, "%s", table[i].text);
            return buf;
        }
    }
    return NULL;  // unreachable
}
