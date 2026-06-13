from r080 import config
from r080.operations import build_operations, OperationStatus


def test_three_modes():
    ops = build_operations()
    assert [o.name for o in ops] == list(config.MODE_TEXTS)


def test_focus_has_no_precision_no_modality():
    ops = build_operations()
    focus = ops[config.FOCUS]
    assert focus.has_precision is False
    assert focus.has_modality is False


def test_countdown_default_seconds():
    ops = build_operations()
    cd = ops[config.COUNTDOWN]
    assert cd.has_precision is True
    assert cd.precision_type == config.SECONDS


def test_teststrip_default_continous_seconds():
    ops = build_operations()
    ts = ops[config.TEST_STRIP]
    assert ts.has_modality is True
    assert ts.modality == config.CONTINOUS
    assert ts.precision_type == config.SECONDS


def test_mode_index_wraps():
    st = OperationStatus(build_operations())
    st.select_by_encoder(3)   # (3 % 3) = 0 → FOCUS
    assert st.current.name == "FOCUS"
    st.select_by_encoder(-1)  # (-1 % 3 + 3) % 3 = 2 → TEST STRIP
    assert st.current.name == "TEST STRIP"


def test_precision_index_wraps_and_sets_value():
    st = OperationStatus(build_operations())
    st.select_by_encoder(config.COUNTDOWN)
    st.set_precision_by_encoder(8)   # SECONDS index 8 → valore 10
    assert st.current.precision_value() == 10


def test_toggle_precision_type():
    st = OperationStatus(build_operations())
    st.select_by_encoder(config.COUNTDOWN)
    assert st.current.precision_type == config.SECONDS
    st.current.toggle_precision_type()
    assert st.current.precision_type == config.FSTOP


def test_toggle_modality_only_teststrip():
    st = OperationStatus(build_operations())
    st.select_by_encoder(config.TEST_STRIP)
    st.current.toggle_modality()
    assert st.current.modality == config.STEP_BY_STEP
