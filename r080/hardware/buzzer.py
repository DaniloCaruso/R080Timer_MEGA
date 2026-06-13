from machine import Pin, PWM
from r080 import config


class Buzzer:
    def __init__(self, pin=config.BUZZER_PIN):
        self._pwm = PWM(Pin(pin), freq=config.TONE_UP, duty=0)
        self._pwm.duty(0)

    def tone(self, freq):
        # avvia un tono continuo; lo stop e' gestito dal SoundPlayer
        self._pwm.freq(freq)
        self._pwm.duty(512)   # 50% su scala 0..1023

    def no_tone(self):
        self._pwm.duty(0)
