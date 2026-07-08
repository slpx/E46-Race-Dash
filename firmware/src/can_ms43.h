// Decode of E46 / MS43 500k CAN frames into VehicleState.
// Pure logic, no hardware dependencies — unit-testable on any host.
// Byte layouts per docs/can-channels.md (MS4X wiki + E46F decode + e46.dbc).
#pragma once
#include "vehicle_state.h"

#ifdef __cplusplus
extern "C" {
#endif

// CAN IDs the dash listens to (11-bit)
#define CANID_DME1     0x316  // rpm, torque, status
#define CANID_DME2     0x329  // coolant, throttle, pedal, brake/clutch
#define CANID_DME4     0x545  // oil temp, warning lights, fuel counter
#define CANID_ASC1     0x153  // ASC status / intervention
#define CANID_WHEELS   0x1F0  // 4x wheel speed
#define CANID_STEER    0x1F5  // steering angle
#define CANID_CLUSTER  0x613  // fuel level, odometer
#define CANID_IKE_AMB  0x615  // ambient temp

// Feed every received frame through this. Returns true if the frame was
// recognised and state updated. now_ms feeds the per-source freshness stamps.
bool ms43_decode(uint32_t id, const uint8_t *data, uint8_t len,
                 VehicleState *s, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
