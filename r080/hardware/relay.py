from machine import Pin
from r080 import config


class Relay:
    def __init__(self, pin=config.RELAY_PIN):
        self.pin = Pin(pin, Pin.OUT)
        self.pin.off()

    def on(self):
        self.pin.on()

    def off(self):
        self.pin.off()

    def set_gate(self, enabled):
        # gate per-canale: accende solo se abilitato (parità con relayStart)
        if enabled:
            self.pin.on()
        else:
            self.pin.off()
