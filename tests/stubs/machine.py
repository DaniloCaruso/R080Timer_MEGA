"""Stub minimale di `machine` per i test host CPython."""


class Pin:
    IN = 0
    OUT = 1
    PULL_UP = 2
    IRQ_FALLING = 4
    IRQ_RISING = 8

    def __init__(self, id, mode=-1, pull=-1, value=None):
        self.id = id
        self.mode = mode
        self.pull = pull
        self._value = 0 if value is None else value
        self._handler = None

    def value(self, v=None):
        if v is None:
            return self._value
        self._value = int(v)

    def on(self):
        self._value = 1

    def off(self):
        self._value = 0

    def irq(self, handler=None, trigger=0):
        self._handler = handler


class I2C:
    def __init__(self, id, scl=None, sda=None, freq=400000):
        self.writes = []  # lista di (addr, bytes)

    def writeto(self, addr, buf):
        self.writes.append((addr, bytes(buf)))

    def writeto_mem(self, addr, memaddr, buf):
        self.writes.append((addr, memaddr, bytes(buf)))

    def scan(self):
        return [0x3E, 0x60]


class UART:
    def __init__(self, id, baudrate=9600, tx=None, rx=None):
        self.sent = bytearray()

    def write(self, buf):
        if isinstance(buf, str):
            buf = buf.encode()
        self.sent.extend(buf)


class PWM:
    def __init__(self, pin, freq=600, duty=0):
        self._freq = freq
        self._duty = duty

    def freq(self, f=None):
        if f is None:
            return self._freq
        self._freq = f

    def duty(self, d=None):
        if d is None:
            return self._duty
        self._duty = d

    def deinit(self):
        self._duty = 0


class Timer:
    ONE_SHOT = 0
    PERIODIC = 1

    def __init__(self, id):
        self.id = id
        self._cb = None

    def init(self, mode=0, period=0, callback=None):
        self._cb = callback

    def deinit(self):
        self._cb = None
