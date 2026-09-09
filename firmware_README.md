# Firmware — ESP32 Wake-Word Inference

Status: **in progress** — Edge Impulse model trained and quantized; on-device flashing and live testing not yet confirmed. This file will be updated once a live test passes.

## Hardware

- ESP32 (WROOM-32 or compatible dev board)
- INMP441 I2S MEMS microphone

### Wiring (typical INMP441 → ESP32 mapping)

| INMP441 pin | ESP32 pin | Notes |
|---|---|---|
| VDD | 3.3V | |
| GND | GND | |
| L/R | GND | Ties mic to left channel |
| WS (word select) | GPIO 15 | Configurable — confirm against `main.cpp` pin defines |
| SCK (bit clock) | GPIO 14 | Configurable — confirm against `main.cpp` pin defines |
| SD (data out) | GPIO 32 | Configurable — confirm against `main.cpp` pin defines |

> Pin numbers above are a common convention, not a hard requirement — cross-check against whatever's actually defined in `src/main.cpp` once wiring is finalized, and update this table to match reality rather than the other way around.

## Software Setup

1. Install [Arduino IDE](https://www.arduino.cc/en/software).
2. Add ESP32 board support via Boards Manager — install version **2.0.x** (later alpha versions have had breaking Wi-Fi/peripheral changes reported by the community; 2.0.x is the tested-compatible line for Edge Impulse ESP32 examples).
3. In Edge Impulse Studio → Deployment, select **Arduino library**, quantized (int8), and Build. Download the resulting `.zip`.
4. In Arduino IDE: `Sketch → Include Library → Add .ZIP Library…`, select the downloaded file.
5. Open `File → Examples → [ProjectName]_inferencing → esp32 → esp32_microphone` as the starting point — it already has the I2S capture + inference loop wired up.
6. Update the I2S pin definitions at the top of the sketch to match the wiring table above (once finalized).

## Build & Flash

1. `Tools → Board` → select your ESP32 board.
2. `Tools → Port` → select the correct serial port.
3. Click Upload.
4. Open `Tools → Serial Monitor` at 115200 baud.
5. Say "TALOS" near the mic and confirm live classification output.

## Current Status / Next Steps

- [ ] Confirm I2S wiring against actual physical connections
- [ ] Flash and verify raw audio capture (no garbage/silence in Serial Monitor)
- [ ] Run live wake-word test, record actual accuracy/false-trigger rate
- [ ] Measure real flash size, RAM (tensor arena), and inference latency on-device
- [ ] Update `MODEL_CARD.md` and main `README.md` with real hardware numbers once available
