from r080 import config

CHANNELS_STR = ("Ch:A", "Ch:B", "Ch:C")


class Presentation:
    """Rendering coordinato di LCD (16x2) e 7-seg. Layout parità con l'originale."""

    def __init__(self, lcd, seg7, state, op_status):
        self.lcd = lcd
        self.seg7 = seg7
        self.state = state
        self.op_status = op_status

    # --- 7 segmenti ---
    def show_time(self, decis):
        self.seg7.show_time(decis)

    # --- LCD ---
    def _write_at(self, col, row, text):
        self.lcd.set_cursor(col, row)
        self.lcd.send_string(text)

    def show_welcome(self):
        self.lcd.clear()
        self._write_at(0, 0, "R080 TIMER")
        self._write_at(0, 1, "PRONTO")

    def render_full(self):
        op = self.op_status.current
        self.lcd.clear()
        # Riga 0: nome modalità + tipo precisione (col 12) + modalità (col 14, solo test strip)
        self._write_at(0, 0, op.name[:11])
        if op.has_precision:
            self._write_at(12, 0, "F" if op.precision_type == config.FSTOP else "S")
        if op.has_modality:
            self._write_at(14, 0, "PP" if op.modality == config.STEP_BY_STEP else "CO")
        self._render_row1()
        self.show_time(self.state.time_set)

    def _render_row1(self):
        op = self.op_status.current
        if op.has_precision:
            self._write_at(0, 1, ("Pr:%s" % op.precision_label())[:8])
        # offset F-stop ai col 5..7
        if op.has_precision and op.precision_type == config.FSTOP:
            self._write_at(5, 1, self.state.fstop.format_offset())
        # canale col 9
        self._write_at(9, 1, CHANNELS_STR[self.state.channel - 1])
        # ♪ col 14 (slot custom 1) se non muto
        self.lcd.set_cursor(14, 1)
        self.lcd.write_char(1 if not self.state.mute else ord(" "))
        # ⚡ col 15 (slot custom 2) se relay attivo sul canale
        self.lcd.set_cursor(15, 1)
        self.lcd.write_char(2 if self.state.channels_relay_activated[self.state.channel - 1] else ord(" "))
