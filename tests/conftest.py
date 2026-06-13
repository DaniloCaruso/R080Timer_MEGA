import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# Gli stub di machine/esp32/uasyncio hanno priorità sui moduli reali (assenti su CPython)
sys.path.insert(0, os.path.join(ROOT, "tests", "stubs"))
sys.path.insert(0, ROOT)
