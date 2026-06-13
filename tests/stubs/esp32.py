"""Stub di `esp32.NVS` in-memory per i test host."""

_STORE = {}


class NVS:
    def __init__(self, namespace):
        self.ns = namespace
        _STORE.setdefault(namespace, {})

    def get_i32(self, key):
        d = _STORE[self.ns]
        if key not in d:
            raise OSError("key not found")
        return d[key]

    def set_i32(self, key, value):
        _STORE[self.ns][key] = int(value)

    def commit(self):
        pass


def _reset():
    _STORE.clear()
