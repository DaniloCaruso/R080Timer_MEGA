import time
from r080 import config

# Sequenza stop: (frequenza_hz, durata_ms); freq 0 = silenzio
STOP_SEQUENCE = (
    (config.TONE_DOWN, 75),
    (0, 150),
    (config.TONE_DOWN, 75),
    (0, 150),
    (config.TONE_DOWN, 150),
)


def _default_clock():
    # Lazy wrapper: funziona su MicroPython (time.ticks_ms) e host (time.time_ns)
    try:
        return time.ticks_ms()
    except AttributeError:
        return time.monotonic_ns() // 1_000_000


class SoundPlayer:
    """State machine non-bloccante per buzzer.

    Parametri injectable per il testing:
      buzzer  — oggetto con .tone(freq) e .no_tone()
      clock   — callable che restituisce ms monotoni
      muted   — callable che restituisce True se il suono è disabilitato
    """

    def __init__(self, buzzer, clock=_default_clock, muted=None):
        self.buzzer = buzzer
        self._clock = clock
        self._muted = muted or (lambda: False)
        self._seq = None
        self._index = 0
        self._started_at = 0
        self._active = False

    # ------------------------------------------------------------------
    # API pubblica
    # ------------------------------------------------------------------

    def emit(self, freq, duration_ms):
        """Beep singolo non-bloccante. No-op se mutato."""
        if self._muted():
            return
        self._seq = ((freq, duration_ms),)
        self._index = 0
        self._started_at = self._clock()
        self._active = True
        self._start_event(self._seq[0])

    def emit_stop(self):
        """Sequenza stop (3 beep TONE_DOWN). Rifiutata se una sequenza è già attiva."""
        if self._active:
            return
        self._seq = STOP_SEQUENCE
        self._index = 0
        self._started_at = self._clock()
        self._active = True
        self._start_event(self._seq[0])

    def tick(self):
        """Avanza la sequenza in base al clock. Da chiamare nel loop principale."""
        if not self._active:
            return
        now = self._clock()
        _, duration = self._seq[self._index]
        if now - self._started_at < duration:
            return
        # Evento corrente scaduto: passa al successivo
        self._index += 1
        if self._index >= len(self._seq):
            self._active = False
            self.buzzer.no_tone()
            return
        self._started_at = now
        self._start_event(self._seq[self._index])

    # ------------------------------------------------------------------
    # Interno
    # ------------------------------------------------------------------

    def _start_event(self, evt):
        freq, _ = evt
        if freq > 0 and not self._muted():
            self.buzzer.tone(freq)
        else:
            self.buzzer.no_tone()
