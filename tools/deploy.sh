#!/usr/bin/env bash
# Carica i file .py sull'ESP32 via mpremote. Uso: tools/deploy.sh <port>
set -euo pipefail
PORT="${1:?porta seriale, es. /dev/ttyUSB0}"
mpremote connect "$PORT" fs cp boot.py :boot.py
mpremote connect "$PORT" fs cp main.py :main.py
mpremote connect "$PORT" fs cp -r r080 :
mpremote connect "$PORT" reset
