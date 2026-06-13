from r080.state import AppState


def test_defaults():
    s = AppState()
    assert s.time_set == 0
    assert s.channel == 1
    assert s.brightness == 10
    assert s.mute is False
    assert s.channel_time == [0, 0, 0]
    assert s.channels_relay_activated == [True, True, True]


def test_current_channel_index_is_zero_based():
    s = AppState()
    s.channel = 3
    assert s.channel_index() == 2
