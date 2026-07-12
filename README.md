# Vantage Wheel

A DIY rotary encoder control module for sim racing — working name "Vantage Wheel". An ESP32-S3 reads a set of rotary encoders and emulates a Bluetooth LE HID gamepad, used as a wireless accessory alongside a Fanatec wheel base (not a replacement for one). A standalone project, independent of the [Button Box](https://github.com/AwsmOli/button-box) — a separate sim racing peripheral. An early sketch briefly lived in that repo before being split out here.

> Status: prototype firmware works — the encoder/gamepad side is implemented and functional.

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

Firmware works; enclosure/mounting design is still to be worked out.

## License

MIT
