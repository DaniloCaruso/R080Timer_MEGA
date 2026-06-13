import time
from machine import Pin
from r080 import config

IDLE = 0
PRESSED = 1
HOLD = 2
RELEASED = 3


def _default_clock():
    # Lazy: time.ticks_ms è chiamato a runtime, non a import-time.
    # Su host CPython l'attributo non esiste ma il modulo importa lo stesso.
    return time.ticks_ms()


class KeyStateMachine:
    """Macchina a stati per singolo tasto, parità con Chris-A Keypad.

    Stati: IDLE → PRESSED → (HOLD) → RELEASED → IDLE
    """

    def __init__(self, debounce_ms=50, hold_ms=500, clock=_default_clock):
        self.debounce_ms = debounce_ms
        self.hold_ms = hold_ms
        self._clock = clock
        self.state = IDLE
        self._down_at = 0
        self._last_change = 0

    def update(self, key, is_down):
        """Aggiorna la macchina a stati.

        :param key: identificatore del tasto (usato solo per parità API)
        :param is_down: True se il tasto è premuto, False se rilasciato
        :return: PRESSED, HOLD, RELEASED oppure None (nessun nuovo evento)
        """
        now = self._clock()

        if is_down and self.state == IDLE:
            self.state = PRESSED
            self._down_at = now
            self._last_change = now
            return PRESSED

        if is_down and self.state == PRESSED:
            if now - self._down_at >= self.hold_ms:
                self.state = HOLD
                return HOLD
            return None

        if is_down and self.state == HOLD:
            # già emesso HOLD, non ripetiamo finché il tasto non viene rilasciato
            return None

        if (not is_down) and self.state in (PRESSED, HOLD):
            self.state = IDLE
            return RELEASED

        return None


class Keypad:
    """Scanner 4x4: righe pilotate (output), colonne lette (input, pull-up esterni 10k).

    Parità hardware con il layout su ESP32-WROOM-32:
      righe → KEYPAD_ROW_PINS (13,14,32,33) — driven LOW durante scansione
      colonne → KEYPAD_COL_PINS (34,35,36,39) — lette, LOW = premuto
    """

    def __init__(self, clock=_default_clock):
        self.rows = [Pin(p, Pin.OUT, value=1) for p in config.KEYPAD_ROW_PINS]
        self.cols = [Pin(p, Pin.IN) for p in config.KEYPAD_COL_PINS]
        self.keymap = config.KEYMAP
        self._sm = {}
        self._clock = clock

    def _scan_down_key(self):
        """Ritorna il primo tasto premuto (colonna LOW con riga attiva-bassa), o None."""
        for r, row in enumerate(self.rows):
            row.value(0)
            for c, col in enumerate(self.cols):
                if col.value() == 0:
                    row.value(1)
                    return self.keymap[r][c]
            row.value(1)
        return None

    def poll(self):
        """Scansiona e aggiorna la macchina a stati.

        :return: (key, event) se c'è un evento, altrimenti (None, None)
        """
        key = self._scan_down_key()
        if key is not None:
            sm = self._sm.setdefault(key, KeyStateMachine(clock=self._clock))
            ev = sm.update(key, True)
            return (key, ev) if ev is not None else (None, None)

        # Nessun tasto giù: chiudi gli stati aperti (rilascio)
        for k, sm in self._sm.items():
            if sm.state != IDLE:
                ev = sm.update(k, False)
                if ev is not None:
                    return (k, ev)

        return (None, None)
