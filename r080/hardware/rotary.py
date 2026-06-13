from machine import Pin


class Rotary:
    """Encoder a quadratura: IRQ su CLK, legge DT. position() ~ read()>>2 dell'originale."""

    def __init__(self, clk_pin, dt_pin):
        self.clk = Pin(clk_pin, Pin.IN, Pin.PULL_UP)
        self.dt = Pin(dt_pin, Pin.IN, Pin.PULL_UP)
        self._pos = 0
        self.clk.irq(handler=self._on_clk, trigger=Pin.IRQ_FALLING)

    def _on_clk(self, _pin):
        # un detent per fronte di discesa del CLK
        if self.dt.value():
            self._pos += 1
        else:
            self._pos -= 1

    def position(self):
        return self._pos
