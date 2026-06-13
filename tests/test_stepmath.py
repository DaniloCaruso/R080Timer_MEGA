from r080.stepmath import next_step_decis


def test_seconds_step_adds_value_times_ten():
    # SECONDS: nuovo = corrente + prec*10 (decis)
    assert next_step_decis(100, is_fstop=False, precision_value=5) == 150


def test_fstop_full_stop_doubles():
    # FSTOP: round(corrente * 2^(1/denom)); denom 1 → ×2
    assert next_step_decis(100, is_fstop=True, precision_value=1) == 200


def test_fstop_min_plus_one():
    # se l'incremento arrotonda allo stesso valore, forzare +1
    assert next_step_decis(1, is_fstop=True, precision_value=64) == 2


def test_clamp_high():
    # 9990 + 5*10 = 10040 → clamp a 9999
    assert next_step_decis(9990, is_fstop=False, precision_value=5) == 9999


def test_clamp_low():
    assert next_step_decis(0, is_fstop=False, precision_value=1) == 10
