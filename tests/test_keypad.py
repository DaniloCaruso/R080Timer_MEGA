from r080.hardware.keypad import KeyStateMachine, IDLE, PRESSED, HOLD, RELEASED


def test_press_then_hold_then_release():
    clock = [0]
    sm = KeyStateMachine(debounce_ms=50, hold_ms=500, clock=lambda: clock[0])

    # tasto giù
    assert sm.update("5", True) == PRESSED
    clock[0] = 30
    assert sm.update("5", True) is None     # entro debounce, nessun nuovo evento
    clock[0] = 600
    assert sm.update("5", True) == HOLD      # superato hold_ms
    clock[0] = 650
    assert sm.update("5", False) == RELEASED


def test_quick_tap_is_released_not_hold():
    clock = [0]
    sm = KeyStateMachine(debounce_ms=50, hold_ms=500, clock=lambda: clock[0])
    assert sm.update("A", True) == PRESSED
    clock[0] = 100
    assert sm.update("A", False) == RELEASED
