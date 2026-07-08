# E46 Race Dash — Firmware

Target: Waveshare ESP32-S3-Touch-LCD-7 (see `../docs/hardware.md`).

## Layout

| File | Role |
|---|---|
| `src/vehicle_state.h` | The one struct everything reads/writes (SI units) |
| `src/can_ms43.*` | E46/MS43 CAN frame decode — pure logic, unit-tested |
| `src/gear_calc.*` | Calculated gear from rpm + wheel speed — pure logic |
| `src/faults.*` | Fault bits + cycling banner text — pure logic |
| `src/sim.*` | Bench simulator (scripted laps, warm-up, ABS flutter) |
| `src/main.cpp` | Tasks: TWAI listen-only RX, 10 Hz serial status |
| `src/board_pins.h` | Pin map — **placeholders, verify against schematic** |
| `test/test_decode/` | Host-side unit tests (`pio test -e native`) |

## Build

```
pip install platformio
pio run                  # build for the board
pio run -t upload        # flash over USB-C
pio device monitor       # watch the 10 Hz status line
pio test -e native       # run unit tests on the PC (needs gcc/clang)
```

`SIM_MODE=1` (default, in `platformio.ini`) runs scripted laps with no car
attached. Set to `0` for live CAN once the transceiver is wired to the bus.

## Milestones

1. **CAN decode + sim → serial** ← this code
2. LVGL UI on the 800×480 panel, layout per `../prototype/dash-alt.html`
3. ADS1115 (oil pressure, battery, turn signals) + MCP2515 (TPMS bus)
4. In-car: verify `TODO(capture)` items in `can_ms43.cpp`, brightness test

## Notes

- The car bus is opened in **listen-only** mode; the dash never ACKs or
  transmits on the MS43/ABS network.
- Decode scalings trace to `../docs/can-channels.md`; anything uncertain is
  marked `TODO(capture)` and needs a live bus dump to confirm.
