import time
from machine import Pin, I2C
from r080 import config

# Comandi LCD
LCD_CLEARDISPLAY = 0x01
LCD_RETURNHOME = 0x02
LCD_ENTRYMODESET = 0x04
LCD_DISPLAYCONTROL = 0x08
LCD_FUNCTIONSET = 0x20
LCD_SETCGRAMADDR = 0x40
LCD_SETDDRAMADDR = 0x80
# flag
LCD_DISPLAYON = 0x04
LCD_ENTRYLEFT = 0x02
LCD_4BITMODE = 0x00
LCD_1LINE = 0x00
LCD_5x8DOTS = 0x00
# registri RGB
REG_MODE1 = 0x00
REG_MODE2 = 0x01
REG_RED = 0x04
REG_GREEN = 0x03
REG_BLUE = 0x02
REG_OUTPUT = 0x08


class LCD1602RGB:
    def __init__(self, i2c=None):
        self.i2c = i2c or I2C(0, scl=Pin(config.I2C_SCL), sda=Pin(config.I2C_SDA), freq=400000)

    def _command(self, value):
        self.i2c.writeto(config.LCD_ADDR, bytes([0x80, value]))

    def _write_data(self, value):
        self.i2c.writeto(config.LCD_ADDR, bytes([0x40, value]))

    def _set_reg(self, reg, data):
        self.i2c.writeto(config.RGB_ADDR, bytes([reg, data]))

    def init(self):
        showfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS
        time.sleep_ms(50)
        for _ in range(3):
            self._command(LCD_FUNCTIONSET | showfunction)
            time.sleep_ms(5)
        self._command(LCD_DISPLAYCONTROL | LCD_DISPLAYON)
        self.clear()
        self._command(LCD_ENTRYMODESET | LCD_ENTRYLEFT)
        # RGB
        self._set_reg(REG_MODE1, 0x00)
        self._set_reg(REG_OUTPUT, 0xFF)
        self._set_reg(REG_MODE2, 0x20)
        self.set_rgb(255, 255, 255)

    def clear(self):
        self._command(LCD_CLEARDISPLAY)
        time.sleep_ms(2)

    def set_rgb(self, r, g, b):
        self._set_reg(REG_RED, r)
        self._set_reg(REG_GREEN, g)
        self._set_reg(REG_BLUE, b)

    def set_cursor(self, col, row):
        addr = col | (0x80 if row == 0 else 0xC0)
        self._command(addr)

    def write_char(self, value):
        self._write_data(value)

    def send_string(self, s):
        for ch in s:
            self._write_data(ord(ch))

    def custom_symbol(self, location, charmap):
        location &= 0x7
        self._command(LCD_SETCGRAMADDR | (location << 3))
        for b in charmap:
            self._write_data(b)
