# E46 Race Dash

ESP32-powered race dash for a BMW E46 race car with an MS43 ECU, styled after
the Porsche Cup car dash (Cosworth: black screen, huge gear digit, shift LED
strip, boxed data fields). Reads the car's 500 kbit/s CAN bus.

## Car configuration (owner-confirmed)

- MS43 (Siemens) DME, MK60 ABS. **Brake pressure NOT available** on the bus.
- Stock fuel tank + sender, dual pump.
- Manual gearbox (gear is calculated, not sensed).
- Display is **not** a touch screen. External dedicated lap timer — no lap
  timing or GPS on this dash, ever.

## Locked design decisions

- Single page, no page switching.
- **Chosen layout: `prototype/dash-alt.html`** (combined gear+speed box +
  4-corner TPMS box). `prototype/dash.html` is the rejected twin-hero variant,
  kept for reference.
- Imperial display units: °F, psi, gal, mph (speed shows no unit label).
  Firmware/state stays SI; convert at render.
- Faults = red flashing banner in the bottom strip, cycling through active
  faults every 1.4 s. Never obscures the main display. No idiot lamps, no
  full-screen takeover (nothing to acknowledge it with).
- ABS box: flashing yellow = intervention, solid red = fault OR wheel-speed
  frame timeout (>500 ms).
- Temps: blue text when cold (water <160 °F, oil <140 °F), red box when hot
  (water ≥217 °F, oil ≥262 °F). Oil pressure red <26 psi above 2500 rpm.
- Car CAN bus is **listen-only** — the dash never transmits on it. TPMS gets
  a second CAN interface (MCP2515).

## Repo map

| Path | What |
|---|---|
| `docs/can-channels.md` | Full CAN message/scaling reference + what's NOT on the bus |
| `docs/hardware.md` | Hardware selection + system architecture |
| `docs/BOM.md` | Shopping list with status tracking |
| `docs/reference/e46.dbc` | Reverse-engineered E46 DBC (SavvyCAN-compatible) |
| `prototype/*.html` | Pixel-reference prototypes (800×480), simulated data |
| `firmware/` | PlatformIO project — see firmware/README.md |

## Build & test

```
cd firmware
pio run -e dash          # ESP32-S3 build (verified working)
pio test -e native       # host unit tests (needs gcc/clang on PATH)
```

## Status & next milestones

Done: prototypes final, hardware selected (Waveshare ESP32-S3-Touch-LCD-7),
firmware milestone 1 (CAN decode + gear calc + faults + bench sim, 11 unit
tests passing), **milestone 2 UI done** — full LVGL UI (`firmware/src/ui/`)
with generated Bahnschrift fonts, verified visually via the host snapshot
harness (`firmware/tools/build_host.py --run` → `firmware/renders/*.bmp`);
both target and host builds green. **Board not yet in hand.**

Next, roughly in order:
1. On board arrival: verify `firmware/src/board_pins.h` against the Waveshare
   schematic (all placeholders), RGB panel driver glue (esp_lcd RGB +
   lv_display flush), call ui_create()/ui_set() from main, brightness test.
2. ADS1115 (oil pressure, battery, turn signals) + MCP2515 (TPMS bus).
3. In-car capture (SavvyCAN + the DBC) to resolve every `TODO(capture)` in
   `firmware/src/can_ms43.cpp`: MK60 ASC intervention bits, 0x615 ambient
   scaling/sign, 0x1F0 upper-nibble bits, 0x153 speed encoding.
4. Fuel: `ui.c` has a placeholder sender curve (`TODO: sender curve`) pending
   the cluster-emulation decision.

## Open decisions (user)

- **TPMS unit** — not yet chosen. When picked: frame IDs/scaling →
  `docs/can-channels.md`, decode → firmware, BOM item 10.
- **Fuel data path** — stock cluster (IKE) must stay electrically connected
  for 0x613 fuel level / 0x615 ambient. "Emulating the stock cluster" was
  explicitly deferred — revisit before wiring. Fallback: senders → ADC.
- **Oil pressure sender mounting** — needs an adapter/tee decision for the
  M54 oil filter housing (BOM item 7).
- Turn-signal arrows are not in the prototype yet (hardwired 12V feeds →
  ADS1115 channels planned; K-Bus deliberately avoided).

## Conventions

- Commit messages end with `Co-Authored-By: Claude Fable 5 <noreply@anthropic.com>`.
- Decode scalings must trace to `docs/can-channels.md`; anything unverified
  is marked `TODO(capture)` in code.
- Prototypes are published as claude.ai artifacts; URLs in the project memory.
