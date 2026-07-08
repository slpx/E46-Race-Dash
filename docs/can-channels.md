# E46 / MS43 CAN Bus Channel Reference

Bus: single K-CAN/PT-CAN on E46, **500 kbit/s**, standard 11-bit IDs, 8-byte frames.
All multi-byte values are **little-endian** (LSB first) unless noted.

Sources: MS4X.net wiki (Siemens MS43 CAN Bus), E46 Fanatics CAN decode thread,
`docs/reference/e46.dbc` (reverse-engineered, andymartell/e46). Byte numbering is 0-based (B0–B7).

---

## Messages from the DME (MS43) — always present

### 0x316 — DME1 (~10 ms)
| Signal | Bytes/Bits | Scaling | Notes |
|---|---|---|---|
| Engine RPM | B2 (LSB), B3 (MSB) | `raw / 6.4` rpm | Primary tach source |
| Torque after interventions | B1 | ~0.39 %/bit | Indicative % of max torque |
| Torque before interventions | B4 | ~0.39 %/bit | |
| Torque loss (consumers) | B5 | ~0.39 %/bit | |
| Ignition on / DME ready | B0 bit 2 | flag | |
| Key on | B0 bit 0 | flag | |
| Starter engaged | B0 bit 4 | flag | |
| A/C clutch | B0 bit 6 | flag | |

### 0x329 — DME2 (~10 ms)
| Signal | Bytes/Bits | Scaling | Notes |
|---|---|---|---|
| Coolant temperature | B1 | `0.75 * raw − 48.373` °C | Key race channel |
| Atmospheric pressure | B2 | `2 * raw + 597` mbar | Ambient, not MAP |
| Clutch switch | B3 bit 0 | flag | Useful for shift detection |
| Engine running | B3 bit 4 | flag | |
| Driver desired torque (pedal) | B4 | 0–254 → 0–100 % | Accelerator pedal position |
| Throttle position | B5 | 0–254 → 0–100 % | Actual throttle plate |
| Brake light switch | B6 bit 0 | flag | |
| Brake switch error | B6 bit 1 | flag | |

### 0x338 — DME3
Sport-mode / duplicate throttle info. Low value for the dash; ignore initially.

### 0x545 — DME4 (~10 ms)
| Signal | Bytes/Bits | Scaling | Notes |
|---|---|---|---|
| Check engine light | B0 bit 1 | flag | |
| Cruise light | B0 bit 3 | flag | |
| EML light | B0 bit 4 | flag | |
| Fuel cap light (2002+) | B0 bit 6 | flag | |
| Fuel consumption counter | B1–B2 | rolling 16-bit counter | Delta/time → instantaneous fuel flow; needs one calibration constant. Enables fuel-per-lap |
| Overheat light | B3 bit 3 | flag | Red coolant warning |
| **Oil temperature** | B4 | `raw − 48.373` °C | From oil condition sensor — reads 0xFF if sensor deleted (aftermarket pan/sump) |
| Charge light | B5 bit 0 | flag | Alternator fault |
| Oil pressure light | B5 bit 4 | flag | Switch only, NOT a pressure value (verify on car) |

---

## Messages from ABS/DSC — present with MK20/MK60 module

### 0x153 — ASC1 (~10 ms)
| Signal | Bytes/Bits | Scaling | Notes |
|---|---|---|---|
| Vehicle speed | B1–B2 (13 bits from bit 11/12) | ~0.125 km/h/bit | Encoding varies by module — prefer wheel speeds below; verify against capture |
| Brake pedal pressed | bit 4 | flag | |
| ASC/DSC intervention | status bits | | Traction event indicator |

### 0x1F0 — Wheel speeds (~10 ms)
| Signal | Bytes/Bits | Scaling |
|---|---|---|
| Front-left wheel speed | bits 0–11 | `raw * 0.0625` km/h |
| Front-right wheel speed | bits 16–27 | `raw * 0.0625` km/h |
| Rear-left wheel speed | bits 32–43 | `raw * 0.0625` km/h |
| Rear-right wheel speed | bits 48–59 | `raw * 0.0625` km/h |

Front/rear axle deltas → wheel-slip / lockup indication. Driven-wheel speed + RPM → **calculated gear**.

### 0x1F5 — Steering angle (LWS, ~10 ms)
| Signal | Bytes/Bits | Scaling |
|---|---|---|
| Steering angle | bits 0–14 + sign bit 15 | `raw * 0.045` ° |
| Steering velocity | bits 16–30 + sign bit 31 | `raw * 0.045` °/s |

### 0x1F3 — Lateral acceleration (MK60 DSC)
Car has MK60, so this should be present; verify scaling from a live capture.
**Brake pressure is NOT available on this car** (confirmed by owner) — don't plan around 0x1F8.

---

## Messages from the instrument cluster (IKE) — ⚠ only if cluster retained

The IKE originates these; if the race car deletes the stock cluster, they disappear.
0x613 also requires the 0x615 handshake to keep transmitting.

### 0x613 (~200 ms)
| Signal | Bytes | Scaling | Notes |
|---|---|---|---|
| Odometer | B0–B1 | ×10 km | |
| Fuel level | B2 | 0x39 = full, 0x08 = low-fuel light, 0x87→0x80 = reserve range | Stock tank + sender retained (dual pump) — usable, but only broadcast while the IKE cluster is in the car |
| Running clock | B3–B4 | minutes | |

### 0x615 (~200 ms)
| Signal | Bytes | Notes |
|---|---|---|
| Ambient temperature | B3 | °C |
| A/C load request, aux fan | various | Not race-relevant |

---

## NOT on the CAN bus — plan add-on sensors

| Channel | Why it matters | Recommended source |
|---|---|---|
| **Oil pressure (value)** | #1 engine-saver channel; CAN only has an idiot-light bit | 0–10 bar analog sender → ESP32 ADC (or ADS1115) |
| **Battery voltage** | Alternator health during race | Resistor divider → ESP32 ADC |
| **Gear position** | Manual box has no sensor | Calculate from RPM ÷ rear-wheel speed lookup |
| **AFR / lambda** | MS43 doesn't broadcast lambda | Wideband controller (analog 0–5 V or its own CAN, e.g. AEM X-Series / Spartan) |
| Fuel level (cell) | Stock sender gone with a fuel cell | Cell's own sender → ADC, or fuel-consumption math from 0x545 |
| Fuel pressure | Optional protection channel | Analog sender → ADC |
| Tire pressures (4 corners) | Puncture/temp monitoring | External CAN TPMS unit (planned) — ESP32 reads its frames off the same bus or a second CAN interface; IDs/scaling per the unit's datasheet |
| Turn signals L/R | Indicator arrows on dash | Not on CAN (they live on the K-Bus body network). Hardwire the 12V indicator feeds → 2 GPIOs via divider/optocoupler. K-Bus tap (TH3122/LIN transceiver on a UART) only worth it if more body data is ever needed |
| Oil temp (if sensor deleted) | 0x545 B4 reads 0xFF without oil condition sensor | Analog NTC → ADC |

---

## Suggested dash channel set (Porsche Cup style)

**Primary (always visible, single page):** RPM (bar + shift LEDs), gear (calculated, huge center
digit), speed (mph, unit not shown), coolant °C, oil °C, oil pressure (ADC), battery voltage (ADC),
ABS box (flashing yellow = intervention, solid red = fault or wheel-speed frame timeout).

**Secondary / alarm-driven:** throttle %, brake flag. All faults (oil pressure, overheat,
CEL/EML/charge, ABS fault) appear in a red flashing banner in the bottom strip, cycling when
multiple are active; critical channels also invert their own box to red. No full-screen
takeover — the display is not a touch screen, so nothing could acknowledge one.
Temps read blue when cold, red-boxed when hot.

**Out of scope / deferred:**
- Lap timing — handled by a dedicated external lap timer unit, not the dash.
- GPS — not used.
- Fuel level/consumption display — deferred; depends on a later decision about keeping or
  emulating the stock cluster (IKE) so 0x613 fuel data stays on the bus.

**Known car config:** stock fuel tank + sender (dual pump), MK60 ABS, no brake pressure signal.

**Open items to verify with a live capture (ESP32 + SavvyCAN dump):**
- 0x153 speed encoding on the MK60
- 0x1F3 lateral accel scaling
- Oil temp validity (oil condition sensor present?)
- Whether the cluster stays in the car (0x613 fuel level needs the IKE alive)
- Fuel consumption counter calibration constant
