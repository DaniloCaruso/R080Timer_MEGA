import time
from machine import Pin


class Button:
    """Pulsante attivo-basso con debounce. update() va chiamato spesso."""

    def __init__(self, pin, debounce_ms=50, click_ms=100, press_ms=1000):
        self.pin = Pin(pin, Pin.IN, Pin.PULL_UP)
        self.debounce_ms = debounce_ms
        self.click_ms = click_ms
        self.press_ms = press_ms
        self._last_level = 1
        self._changed_at = time.ticks_ms()
        self._down_at = 0
        self.on_click = None
        self.on_long_press = None

    def update(self):
        level = self.pin.value()
        now = time.ticks_ms()
        if level != self._last_level:
            if time.ticks_diff(now, self._changed_at) < self.debounce_ms:
                return
            self._changed_at = now
            if level == 0:               # premuto (attivo-basso)
                self._down_at = now
            else:                        # rilasciato
                held = time.ticks_diff(now, self._down_at)
                if held >= self.press_ms and self.on_long_press:
                    self.on_long_press()
                elif held >= self.click_ms and self.on_click:
                    self.on_click()
            self._last_level = level
