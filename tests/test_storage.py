import esp32  # stub
from r080 import config
from r080.state import AppState
from r080.storage import Storage


def setup_function(_):
    esp32._reset()


def test_load_returns_false_on_first_boot():
    st = AppState()
    storage = Storage()
    assert storage.load(st) is False


def test_save_then_load_roundtrip():
    st = AppState()
    st.brightness = 7
    st.mute = True
    st.channel = 2
    st.channel_time = [120, 0, 999]
    st.channels_relay_activated = [True, False, True]
    Storage().save(st)

    st2 = AppState()
    assert Storage().load(st2) is True
    assert st2.brightness == 7
    assert st2.mute is True
    assert st2.channel == 2
    assert st2.channel_time == [120, 0, 999]
    assert st2.channels_relay_activated == [True, False, True]
    # time_set caricato dal canale corrente
    assert st2.time_set == 0          # canale 2 → channel_time[1]
    assert st2.appo_time_set == 0


def test_load_clamps_values():
    nvs = esp32.NVS(config.NVS_NAMESPACE)
    nvs.set_i32("version", config.NVS_VERSION)
    nvs.set_i32("brightness", 99)     # fuori range
    nvs.set_i32("mute", 0)
    nvs.set_i32("channel", 5)         # fuori range
    for i in range(3):
        nvs.set_i32("ch%d_time" % i, 50000)   # fuori range
        nvs.set_i32("ch%d_relay" % i, 1)
    st = AppState()
    assert Storage().load(st) is True
    assert st.brightness == 10
    assert st.channel == 3
    assert st.channel_time == [9999, 9999, 9999]
