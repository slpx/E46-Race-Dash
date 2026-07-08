// Pin map for Waveshare ESP32-S3-Touch-LCD-7.
// !! VERIFY every pin against the Waveshare wiki schematic when the board
// arrives — these are placeholders from preliminary docs reading.
#pragma once

// Onboard CAN transceiver (car bus, listen-only)
#define PIN_CAN_TX   20   // VERIFY
#define PIN_CAN_RX   19   // VERIFY

// I2C header (ADS1115, and the board's own touch/expander share this bus)
#define PIN_I2C_SDA   8   // VERIFY
#define PIN_I2C_SCL   9   // VERIFY

// SPI for MCP2515 (TPMS bus) — must come from free/expansion pins; the RGB
// panel consumes most GPIO. Assign after checking the schematic.
#define PIN_MCP_CS   -1   // TBD
#define PIN_MCP_INT  -1   // TBD
#define PIN_MCP_SCK  -1   // TBD
#define PIN_MCP_MOSI -1   // TBD
#define PIN_MCP_MISO -1   // TBD

// WS2812 shift-light strip (optional) — needs one true GPIO
#define PIN_SHIFT_LEDS -1 // TBD
