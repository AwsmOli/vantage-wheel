# Vantage Wheel

A DIY rotary encoder control module for sim racing — working name "Vantage Wheel". An ESP32-S3 reads a set of rotary encoders and emulates a Bluetooth LE HID gamepad, used as a wireless accessory alongside a Fanatec wheel base (not a replacement for one). A standalone project, independent of the [Button Box](https://github.com/AwsmOli/button-box) — a separate sim racing peripheral. An early sketch briefly lived in that repo before being split out here.

> Status: done, built, and in daily use alongside a Fanatec wheel base. Wish list items: rev lights or a small display.

## What's here

- **`src/main.cpp`** — ESP32-S3 firmware. Reads 5 rotary encoders (with debounced push-buttons) and emulates a 32-button Bluetooth LE HID gamepad via [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad), so it shows up to the PC as a plain Bluetooth gamepad with no companion software or drivers needed.

An earlier prototype explored using the ESP32-S3's USB host mode to read a physical keyboard directly, but the Arduino framework doesn't expose the USB host APIs needed for that — it would require porting to ESP-IDF. The current firmware sticks with Arduino + BLE gamepad emulation instead, and USB host input isn't implemented.

## Hardware

- ESP32-S3 (DevKitC-1, 16MB flash)
- 5 rotary encoders with integrated push-button switches

## Building & Flashing

Requires [PlatformIO](https://platformio.org/) (CLI or the VS Code extension). The [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad) library is pulled automatically via `lib_deps` in `platformio.ini`.

```bash
pio run                      # build
pio run --target upload      # flash over USB
pio device monitor -b 115200 # serial monitor (encoder/BLE debug prints)
```

The board target is `esp32S3` (ESP32-S3-DevKitC-1, 16MB flash) — see `platformio.ini`. After flashing, the device advertises as a Bluetooth LE gamepad named "VantageWheel"; pair it like any other Bluetooth device.

### Pin Configuration

| Encoder | VCC | GND | CLK | DT | SW (button) |
|---|---|---|---|---|---|
| 1 | 18 | 4 | 7 | 6 | 5 |
| 2 | 18 | 17 | 10 | 9 | 8 |
| 3 | 18 | 4 | 2 | 1 | 44 |
| 4 | 37 | 38 | 48 | 35 | 36 |
| 5 | 14 | 4 | 11 | 12 | 13 |

Each encoder's VCC/GND pins are driven directly as digital outputs (`pinMode(..., OUTPUT)`) rather than wired to a fixed rail, so an encoder can be powered from whichever nearby GPIOs are convenient. Pins are defined at the top of `src/main.cpp` (`encoder_vcc_pins`, `encoder_gnd_pins`, `encoder_clk_pins`, `encoder_dt_pins`, `encoder_btn_pins`) if they need to change for a different wiring layout.

## Design Files

- [CAD model (Onshape)](https://cad.onshape.com/documents/08082415ff2de2ac8e2b91c2/w/0c94ee610cfca7954d29ceab/e/e80c9b103543ff9733494459) — enclosure/mounting

## Future Ideas

- Rev lights
- A small display

## License

MIT
