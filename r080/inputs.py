from r080 import config
from r080.hardware.keypad import PRESSED, HOLD, RELEASED


class InputManager:
    """Gestisce input quando il timer e' fermo. Muta state e op_status, salva su NVS."""

    def __init__(self, state, op_status, keypad, enc_mode, enc_prec,
                 enc_mode_btn, enc_prec_btn, presentation, sound, seg7, storage):
        self.state = state
        self.op = op_status
        self.keypad = keypad
        self.enc_mode = enc_mode
        self.enc_prec = enc_prec
        self.enc_mode_btn = enc_mode_btn
        self.enc_prec_btn = enc_prec_btn
        self.pres = presentation
        self.sound = sound
        self.seg7 = seg7
        self.storage = storage
        self._last_mode_pos = None
        self._last_prec_pos = None
        # callback dei pulsanti encoder
        self.enc_mode_btn.on_click = self._on_mode_button
        self.enc_prec_btn.on_click = self._on_prec_button

    # --- encoder ---
    def poll_encoders(self):
        mp = self.enc_mode.position() >> 2
        if mp != self._last_mode_pos:
            self._last_mode_pos = mp
            self.op.select_by_encoder(mp)
            self.pres.render_full()
        pp = self.enc_prec.position() >> 2
        if pp != self._last_prec_pos:
            self._last_prec_pos = pp
            if self.op.current.has_precision:
                self.op.set_precision_by_encoder(pp)
                self._recompute_fstop_if_needed()
                self.pres.render_full()
        self.enc_mode_btn.update()
        self.enc_prec_btn.update()

    def _recompute_fstop_if_needed(self):
        op = self.op.current
        if op.precision_type == config.FSTOP and self.state.fstop.base_time > 0:
            self.state.time_set = self.state.fstop.get_calculated_time(op.precision_value())

    def _on_mode_button(self):
        self.op.current.toggle_modality()
        self.pres.render_full()

    def _on_prec_button(self):
        op = self.op.current
        op.toggle_precision_type()
        if op.precision_type == config.FSTOP:
            self.state.fstop.set_base(self.state.time_set if self.state.time_set > 0 else 1)
        self.pres.render_full()

    # --- keypad ---
    def poll_keypad(self):
        key, ev = self.keypad.poll()
        if key is None:
            return
        if ev == HOLD:
            self._on_hold(key)
        elif ev == RELEASED:
            self._on_released(key)

    def _on_hold(self, key):
        if key == "*":
            self.state.mute = not self.state.mute
            if not self.state.mute:
                self.sound.emit(config.TONE_UP, 200)
            self._save_and_render()
        elif key == "#":
            i = self.state.channel - 1
            self.state.channels_relay_activated[i] = not self.state.channels_relay_activated[i]
            self._save_and_render()

    def _on_released(self, key):
        op = self.op.current
        if key in ("A", "B", "C"):
            self._switch_channel({"A": 1, "B": 2, "C": 3}[key])
        elif key == "D":
            self._reset_channel()
        elif key == "*":
            self._decrement(op)
        elif key == "#":
            self._increment(op)
        elif key.isdigit() and op.name != "FOCUS":
            self._digit(int(key))
        self._save_and_render()

    def _decrement(self, op):
        if op.name == "FOCUS":
            self.state.brightness = max(1, self.state.brightness - 1)
            self.seg7.set_brightness(self.state.brightness)
        elif op.precision_type == config.SECONDS:
            self.state.time_set = max(1, self.state.time_set - op.precision_value() * 10)
        else:  # FSTOP
            self.state.fstop.step_down()
            self.state.time_set = self.state.fstop.get_calculated_time(op.precision_value())

    def _increment(self, op):
        if op.name == "FOCUS":
            self.state.brightness = min(10, self.state.brightness + 1)
            self.seg7.set_brightness(self.state.brightness)
        elif op.precision_type == config.SECONDS:
            self.state.time_set = min(9999, self.state.time_set + op.precision_value() * 10)
        else:  # FSTOP
            self.state.fstop.step_up()
            self.state.time_set = self.state.fstop.get_calculated_time(op.precision_value())

    def _digit(self, d):
        if self.state.n_cifra == 0:
            self.state.time_set = d
        else:
            self.state.time_set = min(9999, self.state.time_set * 10 + d)
        self.state.n_cifra = (self.state.n_cifra + 1) % 4
        self.state.appo_time_set = self.state.time_set
        self.state.fstop.set_base(self.state.time_set)

    def _switch_channel(self, ch):
        self.state.channel_time[self.state.channel - 1] = self.state.time_set
        self.state.channel = ch
        self.state.time_set = self.state.channel_time[ch - 1]
        self.state.appo_time_set = self.state.time_set
        self.state.n_cifra = 0
        self.state.fstop.set_base(self.state.time_set)

    def _reset_channel(self):
        self.state.channel_time[self.state.channel - 1] = 0
        self.state.fstop.reset()
        self.state.time_set = 0
        self.state.appo_time_set = 0

    def _save_and_render(self):
        self.storage.save(self.state)
        self.pres.render_full()
