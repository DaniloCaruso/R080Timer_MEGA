def _constrain(x, lo, hi):
    return lo if x < lo else hi if x > hi else x


class FStopState:
    def __init__(self, base_time=0, stop_offset=0):
        self.base_time = base_time
        self.stop_offset = stop_offset

    def reset(self):
        self.base_time = 0
        self.stop_offset = 0

    def set_base(self, time):
        self.base_time = time
        self.stop_offset = 0

    def get_calculated_time(self, precision_value):
        # precision_value = denominatore (1=full, 2=half, ...)
        if self.base_time <= 0 or precision_value <= 0:
            return self.base_time
        multiplier = 2.0 ** (self.stop_offset / precision_value)
        result = int(self.base_time * multiplier + 0.5)
        return _constrain(result, 1, 9999)

    def step_up(self):
        self.stop_offset += 1

    def step_down(self):
        self.stop_offset -= 1

    def format_offset(self):
        # 3 caratteri: "+02" / "-01" / " 00"
        if self.stop_offset > 0:
            return "+%02d" % min(self.stop_offset, 99)
        if self.stop_offset < 0:
            return "%03d" % max(self.stop_offset, -99)
        return " 00"
