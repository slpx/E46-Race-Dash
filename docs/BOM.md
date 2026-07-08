# Bill of Materials

Status legend: ☐ to order · ◐ ordered · ● in hand

| # | Qty | Part | Role | Est. | Status | Notes |
|---|----|------|------|------|--------|-------|
| 1 | 1 | **Waveshare ESP32-S3-Touch-LCD-7** | Display + MCU + car-bus CAN | $50 | ☐ | 7" 800×480 RGB, ESP32-S3 16MB/8MB, onboard CAN transceiver. Non-touch variant also fine |
| 2 | 1 | Pololu D36V28F5 buck regulator | 12V → 5V 3.2A | $13 | ☐ | 4.5–50V input; survives load dump |
| 3 | 1 | SMBJ33A TVS diode | 12V input clamp | $1 | ☐ | Plus reverse-protection diode (e.g. SS34) |
| 4 | 1 | Inline ATO/mini fuse holder + 2A fuse | 12V input | $2 | ☐ | Tap a switched circuit |
| 5 | 1 | MCP2515 + TJA1050 CAN module | TPMS bus (2nd CAN) | $5 | ☐ | Note crystal freq (usually 8 MHz) for driver config; verify 3.3V logic or substitute MCP2518FD |
| 6 | 1 | ADS1115 breakout (16-bit, 4ch I2C) | Analog: oil P, batt, turn L/R | $6 | ☐ | Power at 3.3V; divide inputs accordingly |
| 7 | 1 | Oil pressure sender 0–100 psi, 0.5–4.5V (AEM 30-2131-100 or equiv) | Oil pressure | $60 | ☐ | 1/8" NPT; needs 5V ratiometric supply |
| 8 | — | Resistors (47k/10k, 33k/10k), 3.3V zeners, 100 nF caps | Input dividers + protection | $3 | ☐ | Battery, turn signals, sender scaling |
| 9 | 1 | WS2812B LED stick ×15 + 74AHCT125 | Physical shift lights (optional) | $10 | ☐ | One GPIO; confirm a free pin on the LCD-7 first |
| 10 | 1 | External CAN TPMS kit | Tire pressures | TBD | ☐ | **User selecting.** Add frame IDs + scaling to can-channels.md when chosen |
| 11 | 1 | Deutsch DTM 12-pos connector kit | Dash ↔ car harness | $15 | ☐ | 12V, GND, CAN_H/L ×2, oil P sig/5V/gnd, turn L/R |
| 12 | — | TXL wire, loom, heat shrink | Harness | $15 | ☐ | |
| 13 | 1 | 3D-printed enclosure + glare hood | Mounting | $10 | ☐ | Design around 192×105 mm board, MXP-style proportions |
| 14 | 1 | MicroSD card (any small) | Data logging (future) | $5 | ☐ | LCD-7 has TF slot; free logging later |

**Total (excl. TPMS): ~$195**

Reference: AiM MXP (the form-factor target) is ~$1,700 without GPS module.

## Order-of-operations

1. Items 1–2 first — everything else can be bench-tested against them.
2. Item 7 (sender) needs a decision on gauge adapter/tee for the M52/M54 oil filter housing.
3. Item 10 (TPMS) drives the second-CAN firmware work; pick before that milestone.
