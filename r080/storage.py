import esp32
from r080 import config


def _constrain(x, lo, hi):
    return lo if x < lo else hi if x > hi else x


class Storage:
    def __init__(self):
        self.nvs = esp32.NVS(config.NVS_NAMESPACE)

    def _get(self, key, default):
        try:
            return self.nvs.get_i32(key)
        except OSError:
            return default

    def _set_if_changed(self, key, value):
        # mirror della semantica eeprom_update: scrive solo se cambiato
        if self._get(key, None) != value:
            self.nvs.set_i32(key, value)

    def load(self, state):
        if self._get("version", None) != config.NVS_VERSION:
            return False
        state.brightness = _constrain(self._get("brightness", 10), 1, 10)
        state.mute = self._get("mute", 0) != 0
        state.channel = _constrain(self._get("channel", 1), 1, 3)
        for i in range(3):
            state.channel_time[i] = _constrain(self._get("ch%d_time" % i, 0), 0, 9999)
            state.channels_relay_activated[i] = self._get("ch%d_relay" % i, 1) != 0
        state.time_set = state.channel_time[state.channel - 1]
        state.appo_time_set = state.time_set
        return True

    def save(self, state):
        self._set_if_changed("version", config.NVS_VERSION)
        self._set_if_changed("brightness", int(state.brightness))
        self._set_if_changed("mute", 1 if state.mute else 0)
        self._set_if_changed("channel", int(state.channel))
        for i in range(3):
            self._set_if_changed("ch%d_time" % i, int(state.channel_time[i]))
            self._set_if_changed("ch%d_relay" % i, 1 if state.channels_relay_activated[i] else 0)
        self.nvs.commit()
