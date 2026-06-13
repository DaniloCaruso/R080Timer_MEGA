class Seg7:
    """Display seriale stile SparkFun, 9600 baud, su UART iniettabile."""

    CMD_RESET = 0x76
    CMD_DP = 0x77
    CMD_BRIGHTNESS = 0x7A
    DP_MASK_POS1 = 0b00000100

    def __init__(self, uart):
        self.uart = uart

    def reset(self):
        self.uart.write(bytes([self.CMD_RESET]))

    def set_brightness(self, value):
        self.uart.write(bytes([self.CMD_BRIGHTNESS, value & 0xFF]))

    def show_time(self, decis):
        decis = 0 if decis < 0 else 9999 if decis > 9999 else decis
        self.uart.write(bytes([self.CMD_DP, self.DP_MASK_POS1]))
        self.uart.write(("%04d" % decis).encode())
