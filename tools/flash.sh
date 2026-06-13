#!/usr/bin/env bash
# Flash del firmware MicroPython sull'ESP32. Uso: tools/flash.sh <port> <firmware.bin>
set -euo pipefail
PORT="${1:?porta seriale, es. /dev/ttyUSB0}"
FW="${2:?path al firmware MicroPython .bin}"
esptool.py --chip esp32 --port "$PORT" erase_flash
esptool.py --chip esp32 --port "$PORT" --baud 460800 write_flash -z 0x1000 "$FW"
