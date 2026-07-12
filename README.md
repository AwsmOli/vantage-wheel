# Vantage Wheel

A DIY sim racing wheel base — working name "Vantage Wheel". An ESP32-S3 reads a set of rotary encoders and emulates a Bluetooth LE HID gamepad, pairing with the [Button Box](https://github.com/AwsmOli/button-box) sim racing button box.

> Status: early idea, not fleshed out yet — prototype firmware exists, but the wheel itself (force feedback, motor/driver, rim) isn't designed yet.

## What's here

- **`src/main.cpp`** — ESP32-S3 firmware. Reads 5 rotary encoders (with debounced push-buttons) and emulates a 32-button Bluetooth LE HID gamepad via [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad), so it shows up to the PC as a plain Bluetooth gamepad with no companion software or drivers needed.

An earlier prototype explored using the ESP32-S3's USB host mode to read a physical keyboard directly, but the Arduino framework doesn't expose the USB host APIs needed for that — it would require porting to ESP-IDF. The current firmware sticks with Arduino + BLE gamepad emulation instead, and USB host input isn't implemented.

## Hardware

- ESP32-S3 (DevKitC-1, 16MB flash)
- 5 rotary encoders with integrated push-button switches

## Building

```bash
pio run              # build
pio run --target upload
```

## Roadmap

Force feedback approach, motor/driver selection, and the wheel rim itself are still to be worked out — this repo currently covers the encoder/button-box side of the wheel base, not the actual force feedback hardware.

## License

MIT
