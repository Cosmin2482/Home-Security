
# Home Security Device (Arduino)

Minimal, reliable Arduino security prototype with **PIR motion** + **door reed** sensors, **ARM/DISARM hold button**, **buzzer siren**, and **RGB LED** status.

## Features
- States: **DISARMED → EXIT_DELAY → ARMED → ENTRY_DELAY → ALARM**
- Non-blocking timing with `millis()`, debounced inputs
- Hold **button (D4)** for **2 seconds** to ARM/DISARM
- Configurable delays (`EXIT_DELAY_MS`, `ENTRY_DELAY_MS`, `ALARM_TIME_MS`)
- Reed switch **NC/NO** supported (`REED_NC` flag)

## Hardware (default pins)
- PIR → **D2**, 5V, GND
- Reed → **D3** to **GND** (uses **INPUT_PULLUP**). If reed is **NC**, set `REED_NC=true`.
- Button → **D4** to **GND** (INPUT_PULLUP)
- RGB LED: **R=D5**, **G=D6** (220Ω resistors to LED → GND)
- Buzzer → **D10** (+) (optional 100Ω), GND

## How it works
- **Arm**: Hold button 2s → **EXIT_DELAY** (slow beeps) → **ARMED** (red LED, triple chirp)
- **Breach**: Door open or PIR motion → **ENTRY_DELAY** (fast beeps) → **ALARM** (warble siren)
- **Disarm**: Hold button 2s in any state → **DISARMED** (green LED, double chirp)

## Customize
Edit these constants at the top of the sketch:
```cpp
const unsigned long EXIT_DELAY_MS  = 15000UL;
const unsigned long ENTRY_DELAY_MS = 10000UL;
const unsigned long ALARM_TIME_MS  = 60000UL;
const bool REED_NC = false; // set true for Normally-Closed reed
```

## Build & Upload
1. Open `src/HomeSecurityDevice.ino` in **Arduino IDE**
2. Select your board (e.g., **Arduino UNO**) and the correct COM port
3. Verify & Upload

## Ideas to Extend
- Keypad/RFID disarm code
- ESP8266/ESP32 push notifications (Wi‑Fi)
- Event logging to SD card
- Add Blue channel + night light dimming

## License
MIT
