from r080 import config


def test_precision_arrays_length():
    assert len(config.FSTOP_PRECISIONS) == config.PRECISION_COUNT
    assert len(config.SECONDS_PRECISIONS) == config.PRECISION_COUNT


def test_fstop_denominators():
    vals = [v for v, _ in config.FSTOP_PRECISIONS]
    assert vals == [1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 64]


def test_seconds_steps():
    vals = [v for v, _ in config.SECONDS_PRECISIONS]
    assert vals == [1, 2, 3, 4, 5, 6, 7, 8, 10, 15, 20]


def test_keymap_shape():
    assert config.KEYMAP[3] == ("*", "0", "#", "D")
