import sys
import types

import pytest


@pytest.fixture(autouse=True)
def _restore_time():
    """Ripristina il modulo time reale dopo ogni test che installa un fake."""
    import time as _real_time
    saved = sys.modules.get("time")
    yield
    if saved is not None:
        sys.modules["time"] = _real_time
    else:
        sys.modules.pop("time", None)
    # Rimuovi anche la cache del modulo engine in modo che il prossimo test
    # possa ricaricare con il time corretto.
    sys.modules.pop("r080.engine", None)


def _install_fake_time(t):
    """Installa un modulo `time` finto che legge il tick da t[0] (ms)."""
    fake = types.ModuleType("time")
    fake.ticks_ms = lambda: t[0]
    fake.ticks_add = lambda a, b: a + b
    fake.ticks_diff = lambda a, b: a - b
    sys.modules["time"] = fake


def _load_engine():
    import importlib
    import r080.engine as eng
    importlib.reload(eng)
    return eng


def test_focus_counts_up():
    t = [0]
    _install_fake_time(t)
    eng = _load_engine()
    rendered = []
    e = eng.TimingEngine(
        render=lambda d: rendered.append(d),
        on_metronome=lambda: None,
        on_countdown_end=lambda: None,
        on_step=lambda n: None,
        on_step_warning=lambda: None,
        relay_off=lambda: None,
    )
    e.start_focus()
    e._tick_100ms()
    e._tick_100ms()
    assert rendered == [1, 2]


def test_countdown_ends_and_calls_relay_off():
    t = [0]
    _install_fake_time(t)
    eng = _load_engine()
    calls = {"relay": 0, "end": 0}
    e = eng.TimingEngine(
        render=lambda d: None,
        on_metronome=lambda: None,
        on_countdown_end=lambda: calls.__setitem__("end", calls["end"] + 1),
        on_step=lambda n: None,
        on_step_warning=lambda: None,
        relay_off=lambda: calls.__setitem__("relay", calls["relay"] + 1),
    )
    e.start_countdown(10, gate_enabled=False)   # 10 decis = 1.0 s
    t[0] = 1100                                  # tempo scaduto
    e._tick_100ms()
    assert calls["relay"] == 1
    assert calls["end"] == 1
    assert e.mode == eng.MODE_NONE


def test_countdown_metronome_fires_on_second_boundary():
    t = [0]
    _install_fake_time(t)
    eng = _load_engine()
    beats = []
    e = eng.TimingEngine(
        render=lambda d: None,
        on_metronome=lambda: beats.append(t[0]),
        on_countdown_end=lambda: None,
        on_step=lambda n: None,
        on_step_warning=lambda: None,
        relay_off=lambda: None,
    )
    e.start_countdown(30, gate_enabled=False)   # 3.0 s
    # A t=0 rem_ms=3000, sec=3  → primo beat
    e._tick_100ms()
    assert len(beats) == 1
    # t=100 rem_ms=2900, sec=2  → cambio secondo → altro beat
    t[0] = 100
    e._tick_100ms()
    assert len(beats) == 2
    # t=200 rem_ms=2800, sec=2  → stessa sec → nessun beat
    t[0] = 200
    e._tick_100ms()
    assert len(beats) == 2


def test_test_strip_continous_advances_target():
    t = [0]
    _install_fake_time(t)
    eng = _load_engine()
    steps = []
    e = eng.TimingEngine(
        render=lambda d: None,
        on_metronome=lambda: None,
        on_countdown_end=lambda: None,
        on_step=lambda n: steps.append(n),
        on_step_warning=lambda: None,
        relay_off=lambda: None,
    )
    # Partenza: appo_decis=10, SECONDS 1s precision (precision_value=1 → step=10 decis)
    e.start_test_strip(appo_decis=10, is_fstop=False, precision_value=1,
                       modality_step=False, resume=False)
    assert e.next_step_target == 10
    # Simula 1000 ms di avanzamento → elapsed=1000ms → current=10 decis → raggiunge target
    t[0] = 1000
    e._tick_100ms()
    assert len(steps) == 1
    # Nuovo target = 10 + 10 = 20
    assert steps[0] == 20
    assert e.next_step_target == 20
    assert e.mode == eng.MODE_TEST_STRIP   # CONTINOUS → non si ferma


def test_test_strip_step_by_step_pauses():
    t = [0]
    _install_fake_time(t)
    eng = _load_engine()
    relay_calls = []
    e = eng.TimingEngine(
        render=lambda d: None,
        on_metronome=lambda: None,
        on_countdown_end=lambda: None,
        on_step=lambda n: None,
        on_step_warning=lambda: None,
        relay_off=lambda: relay_calls.append(1),
    )
    e.start_test_strip(appo_decis=10, is_fstop=False, precision_value=1,
                       modality_step=True, resume=False)
    t[0] = 1000
    e._tick_100ms()
    # STEP_BY_STEP: relay_off chiamato e mode → NONE
    assert len(relay_calls) == 1
    assert e.mode == eng.MODE_NONE


def test_test_strip_warning_fires_within_3_decis():
    t = [0]
    _install_fake_time(t)
    eng = _load_engine()
    warnings = []
    e = eng.TimingEngine(
        render=lambda d: None,
        on_metronome=lambda: None,
        on_countdown_end=lambda: None,
        on_step=lambda n: None,
        on_step_warning=lambda: warnings.append(1),
        relay_off=lambda: None,
    )
    # target=10, warning se dist ∈ [1..3]
    e.start_test_strip(appo_decis=10, is_fstop=False, precision_value=1,
                       modality_step=False, resume=False)
    # elapsed=600ms → current=6 decis, dist=4 → nessun warning
    t[0] = 600
    e._tick_100ms()
    assert warnings == []
    # elapsed=700ms → current=7 decis, dist=3 → warning
    t[0] = 700
    e._tick_100ms()
    assert len(warnings) == 1


def test_stop_sets_mode_none():
    t = [0]
    _install_fake_time(t)
    eng = _load_engine()
    e = eng.TimingEngine(
        render=lambda d: None, on_metronome=lambda: None,
        on_countdown_end=lambda: None, on_step=lambda n: None,
        on_step_warning=lambda: None, relay_off=lambda: None,
    )
    e.start_focus()
    assert e.mode == eng.MODE_FOCUS
    e.stop()
    assert e.mode == eng.MODE_NONE
