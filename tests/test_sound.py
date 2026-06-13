from r080 import config
from r080.sound import SoundPlayer


class FakeBuzzer:
    def __init__(self):
        self.events = []   # ("tone", freq) | ("off",)

    def tone(self, freq):
        self.events.append(("tone", freq))

    def no_tone(self):
        self.events.append(("off",))


def test_single_beep_starts_and_stops():
    clock = [0]
    bz = FakeBuzzer()
    sp = SoundPlayer(bz, clock=lambda: clock[0])
    sp.emit(config.TONE_UP, 150)
    assert ("tone", config.TONE_UP) in bz.events
    clock[0] = 160
    sp.tick()
    assert bz.events[-1] == ("off",)


def test_muted_emits_nothing():
    bz = FakeBuzzer()
    sp = SoundPlayer(bz, clock=lambda: 0, muted=lambda: True)
    sp.emit(config.TONE_UP, 150)
    assert bz.events == []


def test_stop_sequence_plays_five_events():
    clock = [0]
    bz = FakeBuzzer()
    sp = SoundPlayer(bz, clock=lambda: clock[0])
    sp.emit_stop()
    for t in range(0, 700, 10):
        clock[0] = t
        sp.tick()
    tone_events = [e for e in bz.events if e[0] == "tone"]
    assert len(tone_events) == 3   # 3 beep TONE_DOWN nella sequenza


def test_stop_refused_while_active():
    clock = [0]
    bz = FakeBuzzer()
    sp = SoundPlayer(bz, clock=lambda: clock[0])
    sp.emit_stop()
    n = len(bz.events)
    sp.emit_stop()   # rifiutato: sequenza in corso
    assert len(bz.events) == n
