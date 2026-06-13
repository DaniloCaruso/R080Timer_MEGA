from r080.fstop import FStopState


def test_zero_base_short_circuits():
    s = FStopState()
    assert s.get_calculated_time(2) == 0  # base 0 → ritorna base


def test_full_stop_up_doubles():
    s = FStopState()
    s.set_base(100)        # 10.0 s
    s.step_up()            # +1 stop, denom 1 → ×2
    assert s.get_calculated_time(1) == 200


def test_full_stop_down_halves():
    s = FStopState()
    s.set_base(100)
    s.step_down()          # -1 stop, denom 1 → ÷2
    assert s.get_calculated_time(1) == 50


def test_half_stop_rounds():
    s = FStopState()
    s.set_base(100)
    s.step_up()            # +1 a denom 2 → ×2^(1/2)=1.4142 → 141.42 → 141
    assert s.get_calculated_time(2) == 141


def test_clamp_high():
    s = FStopState()
    s.set_base(9000)
    s.step_up()
    assert s.get_calculated_time(1) == 9999  # 18000 clampato


def test_clamp_low_never_zero():
    s = FStopState()
    s.set_base(1)
    s.step_down()
    s.step_down()
    assert s.get_calculated_time(1) == 1  # mai 0 (silenzierebbe l'ingranditore)


def test_set_base_resets_offset():
    s = FStopState()
    s.set_base(100)
    s.step_up()
    s.set_base(200)
    assert s.stop_offset == 0


def test_format_offset():
    s = FStopState()
    assert s.format_offset() == " 00"
    s.step_up(); s.step_up()
    assert s.format_offset() == "+02"
    s.set_base(100); s.step_down()
    assert s.format_offset() == "-01"
