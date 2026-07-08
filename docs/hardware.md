# Hardware Selection

Target form factor: AiM MXP class — 6" 800×480 display, unit 189.6 × 106.4 × 24.9 mm.
The prototype (`prototype/dash-alt.html`) is designed at 800×480, so the chosen panel
renders it pixel-for-pixel.

## Display + MCU (the big decision)

**Recommended: Waveshare ESP32-S3-Touch-LCD-7** (~$45–55)

| Spec | Value | vs MXP |
|---|---|---|
| Panel | 7" IPS RGB, 800×480 | Same resolution; ~1" larger diagonal |
| Board outline | ~192 × 105 mm | Nearly identical to MXP's 189.6 × 106.4 |
| MCU | ESP32-S3 (dual LX7 @ 240 MHz), 16 MB flash, 8 MB PSRAM | Plenty for LVGL at 800×480 |
| CAN | **Onboard transceiver + screw terminal** | Car bus needs zero extra hardware |
| Extras | RS485, I2C header, TF card slot, USB-C | TF card = free data logging later |

Why this one: the board *is* the dash — display, MCU, and the car-bus CAN
transceiver on one PCB the size of an MXP, so the custom electronics shrink to
power + sensors. A non-touch variant exists (we don't need touch); either works.

Alternatives considered:
- **Waveshare ESP32-S3-Touch-LCD-5** (5", 800×480, also has CAN) — MXS-sized, if the 7" won't fit the pod.
- **Elecrow CrowPanel Advance 7"** — similar class; evaluate only if Waveshare availability is a problem.
- **Discrete build** (ESP32-S3 devkit + raw RGB panel) — full GPIO freedom, but 40-pin FPC wiring and a custom PCB; not worth it unless we hit the GPIO wall (see Open items).

**Known compromise — brightness.** Hobby panels run ~400–500 nits; the MXP's
sunlight-readable panel is 700+ with an ambient light sensor. In a closed E46 with
a printed anti-glare hood this should be acceptable; the black-background Cup design
helps (only the digits emit light). If track testing proves it too dim, the escape
path is a high-brightness (850–1000 nit) 7" RGB panel swapped onto a discrete build.

## System architecture

```
                     ┌──────────────────────────────────────┐
 12V switched ──fuse─┤ TVS   5V buck (Pololu D36V28F5)      │
                     │        │                             │
 Car CAN (tap at ────┤ onboard CAN (TWAI, LISTEN-ONLY)      │
 cluster connector)  │                                      │
                     │  ESP32-S3-Touch-LCD-7  ──── 800×480  │
 TPMS CAN bus ───────┤ MCP2515 (SPI)                        │
                     │                                      │
 Oil press 0.5–4.5V ─┤ ADS1115 ch0 ─┐                       │
 Battery 12V ──div───┤ ADS1115 ch1  ├─ I2C                  │
 Turn L 12V ──div────┤ ADS1115 ch2  │                       │
 Turn R 12V ──div────┤ ADS1115 ch3 ─┘                       │
                     │                                      │
 Shift LED strip ────┤ GPIO → WS2812B ×15 (optional)        │
                     └──────────────────────────────────────┘
```

Two CAN buses by design:
- **Car bus: listen-only.** The dash never transmits on the MS43/ABS bus — zero risk
  of disturbing the powertrain. Onboard transceiver handles it.
- **TPMS bus: separate MCP2515 SPI module** (~$5). Keeps an aftermarket transmitter
  off the engine bus entirely, and gives us a place to hang future accessories.

**CAN tap point:** the E46 OBD port does NOT expose CAN (K-line only). Tap CAN_H/CAN_L
at the instrument cluster connector or the DME harness.

## Bill of materials

| Item | Part | ~Cost | Notes |
|---|---|---|---|
| Display/MCU | Waveshare ESP32-S3-Touch-LCD-7 | $50 | Core of the build |
| Power | Pololu D36V28F5 buck (4.5–50V in, 5V 3.2A) | $13 | 50V input ceiling survives load dump |
| Input protection | SMBJ33A TVS, inline fuse (2A), reverse diode | $3 | At the 12V entry |
| 2nd CAN | MCP2515 + TJA1050 module (8 MHz crystal) | $5 | For TPMS; note crystal freq for driver config |
| ADC | ADS1115 module (16-bit, 4ch, I2C) | $6 | Oil pressure, battery, turn signals |
| Oil pressure sender | AEM 30-2131-100 (0–100 psi, 0.5–4.5V) or equiv | $60 | Ratiometric 5V; 1/8" NPT |
| Dividers/protection | Resistors, zeners, 100 nF caps | $3 | Battery + turn signal + sender scaling |
| Shift LEDs (optional) | WS2812B stick ×15 + 74AHCT125 level shifter | $10 | Physical LEDs above screen, Cup style |
| TPMS | User-selected external CAN TPMS (e.g. IZZE-Racing class) | TBD | Frame IDs/scaling → this doc when chosen |
| Enclosure | 3D-printed ASA/PETG, MXP-style + glare hood | $10 | Model around 192×105 board |
| Harness | Deutsch DTM connector, TXL wire, loom | $30 | One connector to the car |

Total (excl. TPMS): **~$190** — against ~$1,700 for an MXP.

## Open items to verify on hardware arrival

1. **Free GPIO count** on the LCD-7 — the RGB panel consumes most pins (some IO is
   behind an I2C expander). Turn signals are planned onto ADS1115 channels for this
   reason (a 1.5 Hz 12V square wave reads fine at 10 Hz sampling). The WS2812 strip
   needs one true GPIO — confirm one is free; if not, LEDs stay on-screen only.
2. ADS1115 powered at 3.3V → scale the 0.5–4.5V sender to <3.3V with a divider
   (or power at 5V and level-shift I2C). Decide at wiring time.
3. MCP2515 modules are 5V boards — confirm 3.3V logic compatibility or fit an
   MCP2518FD module instead.
4. Panel brightness in-car — test before final enclosure.
