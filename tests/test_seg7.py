import machine
from r080.hardware.seg7 import Seg7


def test_show_time_sends_dp_and_digits():
    uart = machine.UART(2, baudrate=9600, tx=17)
    seg = Seg7(uart)
    seg.show_time(100)   # 10.0
    # atteso: 0x77, maschera DP cifra1, poi "0100"
    assert uart.sent[0] == 0x77
    assert uart.sent[1] == 0b00000100
    assert bytes(uart.sent[2:6]) == b"0100"


def test_reset_sends_0x76():
    uart = machine.UART(2, baudrate=9600, tx=17)
    seg = Seg7(uart)
    seg.reset()
    assert uart.sent[0] == 0x76


def test_set_brightness():
    uart = machine.UART(2, baudrate=9600, tx=17)
    seg = Seg7(uart)
    uart.sent = bytearray()
    seg.set_brightness(7)
    assert uart.sent[0] == 0x7A
    assert uart.sent[1] == 7
