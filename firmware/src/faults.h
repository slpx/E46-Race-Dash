// Fault evaluation + cycling banner text, exactly mirroring the HTML
// prototype's behavior: red flashing banner in the bottom strip, cycling
// through active faults every FAULT_CYCLE_MS. Pure logic, unit-testable.
#pragma once
#include "vehicle_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FAULT_OIL_PRESSURE = 1 << 0,   // < 1.4 bar above 3000 rpm (ADC value)
    FAULT_OVERHEAT     = 1 << 1,   // coolant >= 106 C or DME overheat lamp
    FAULT_CEL          = 1 << 2,
    FAULT_EML          = 1 << 3,
    FAULT_CHARGE       = 1 << 4,   // DME charge lamp or battery < 12.6 V
    FAULT_ABS          = 1 << 5,   // ABS frames stopped (>500 ms)
    FAULT_TIRE         = 1 << 6,   // any tire < 26 psi (valid data only)
    FAULT_NO_DME       = 1 << 7,   // DME frames stopped -> bus/ignition issue
} FaultBits;

#define FAULT_CYCLE_MS   1400
#define SRC_TIMEOUT_MS   500

uint32_t faults_eval(const VehicleState *s, uint32_t now_ms);

// Banner text for the current cycle position; NULL when no faults.
// For FAULT_TIRE the string names low corners, e.g. "TIRE FR".
const char *faults_banner_text(uint32_t faults, const VehicleState *s,
                               uint32_t now_ms, char *buf, int buflen);

#ifdef __cplusplus
}
#endif
