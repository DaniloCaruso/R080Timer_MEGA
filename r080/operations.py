from r080 import config


class Operation:
    def __init__(self, name, has_precision=False, has_modality=False):
        self.name = name
        self.has_precision = has_precision
        self.has_modality = has_modality
        self.precision_type = config.SECONDS if has_precision else config.NO_PRECISION
        self.precision_index = 0
        self.modality = config.CONTINOUS if has_modality else config.NO_MODALITY

    def precision_table(self):
        if self.precision_type == config.FSTOP:
            return config.FSTOP_PRECISIONS
        return config.SECONDS_PRECISIONS

    def precision_value(self):
        if not self.has_precision:
            return 0
        return self.precision_table()[self.precision_index][0]

    def precision_label(self):
        if not self.has_precision:
            return ""
        return self.precision_table()[self.precision_index][1]

    def set_precision_index(self, idx):
        self.precision_index = idx % config.PRECISION_COUNT

    def toggle_precision_type(self):
        if not self.has_precision:
            return
        self.precision_type = (
            config.FSTOP if self.precision_type == config.SECONDS else config.SECONDS
        )

    def toggle_modality(self):
        if not self.has_modality:
            return
        self.modality = (
            config.STEP_BY_STEP if self.modality == config.CONTINOUS else config.CONTINOUS
        )


def build_operations():
    return [
        Operation(config.MODE_TEXTS[config.FOCUS]),
        Operation(config.MODE_TEXTS[config.COUNTDOWN], has_precision=True),
        Operation(config.MODE_TEXTS[config.TEST_STRIP], has_precision=True, has_modality=True),
    ]


class OperationStatus:
    def __init__(self, operations):
        self.operations = operations
        self.current = operations[config.FOCUS]
        self.last = operations[config.FOCUS]

    def select_by_encoder(self, raw_pos):
        idx = (raw_pos % 3 + 3) % 3
        self.last = self.current
        self.current = self.operations[idx]

    def set_precision_by_encoder(self, raw_pos):
        idx = (raw_pos % config.PRECISION_COUNT + config.PRECISION_COUNT) % config.PRECISION_COUNT
        self.current.set_precision_index(idx)
