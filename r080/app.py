import time
import uasyncio as asyncio
from machine import Pin, UART
from r080 import config
from r080.state import AppState
from r080.operations import build_operations, OperationStatus
from r080.storage import Storage
from r080.sound import SoundPlayer
from r080.engine import TimingEngine
from r080.presentation import Presentation
from r080.inputs import InputManager
from r080.hardware.relay import Relay
from r080.hardware.buzzer import Buzzer
from r080.hardware.seg7 import Seg7
from r080.hardware.lcd1602_rgb import LCD1602RGB
from r080.hardware.keypad import Keypad
from r080.hardware.rotary import Rotary
from r080.hardware.button import Button


class App:
    def __init__(self):
        self.state = AppState()
        self.op = OperationStatus(build_operations())
        # hardware
        self.relay = Relay()
        self.buzzer = Buzzer()
        self.seg7 = Seg7(UART(2, baudrate=9600, tx=config.SEG7_TX))
        self.lcd = LCD1602RGB()
        self.keypad = Keypad()
        self.enc_mode = Rotary(config.ENC_MODE_CLK, config.ENC_MODE_DT)
        self.enc_prec = Rotary(config.ENC_PREC_CLK, config.ENC_PREC_DT)
        self.enc_mode_btn = Button(config.ENC_MODE_SW)
        self.enc_prec_btn = Button(config.ENC_PREC_SW)
        self.main_btn = Button(config.MAIN_BTN_PIN)
        self.storage = Storage()
        # software
        self.sound = SoundPlayer(self.buzzer, muted=lambda: self.state.mute)
        self.pres = Presentation(self.lcd, self.seg7, self.state, self.op)
        self.engine = TimingEngine(
            render=self.pres.show_time,
            on_metronome=lambda: self.sound.emit(config.TONE_DOWN, 80),
            on_countdown_end=self._on_countdown_end,
            on_step=self._on_step,
            on_step_warning=lambda: self.sound.emit(config.TONE_UP, 50),
            relay_off=self.relay.off,
        )
        self.inputs = InputManager(
            self.state, self.op, self.keypad, self.enc_mode, self.enc_prec,
            self.enc_mode_btn, self.enc_prec_btn, self.pres, self.sound,
            self.seg7, self.storage,
        )
        self.main_btn.on_click = self._toggle_main

    def _toggle_main(self):
        self.state.is_main_btn_pressed = not self.state.is_main_btn_pressed

    def _on_countdown_end(self):
        self.state.is_action_started = False
        self.state.is_main_btn_pressed = False
        self.sound.emit_stop()
        self.state.time_set = self.state.appo_time_set   # repeat-last-exposure

    def _on_step(self, new_target):
        self.sound.emit(config.TONE_UP, 150)
        op = self.op.current
        if op.modality == config.STEP_BY_STEP:
            self.state.is_action_started = False
            self.state.is_main_btn_pressed = False
            self.pres.show_time(new_target)

    def _start_action(self):
        op = self.op.current
        # Countdown a tempo zero: non avviare e non energizzare il relay (parità con startCountdownTimer)
        if op.name == "COUNTDOWN" and self.state.appo_time_set <= 0:
            self.state.is_main_btn_pressed = False
            return
        gate = self.state.channels_relay_activated[self.state.channel - 1]
        self.relay.set_gate(gate)
        self.state.is_action_started = True
        if op.name == "FOCUS":
            self.engine.start_focus()
        elif op.name == "COUNTDOWN":
            self.engine.start_countdown(self.state.appo_time_set, gate)
        else:  # TEST STRIP
            resume = (op.modality == config.STEP_BY_STEP and self.engine.current_decis > 0)
            self.engine.start_test_strip(
                self.state.appo_time_set,
                op.precision_type == config.FSTOP,
                op.precision_value(),
                op.modality == config.STEP_BY_STEP,
                resume,
            )

    def _stop_action(self):
        self.engine.stop()
        self.relay.off()
        self.state.is_action_started = False
        self.sound.emit_stop()
        self.pres.show_time(self.state.time_set)

    def setup(self):
        self.relay.off()
        self.lcd.init()
        self.lcd.set_rgb(255, 0, 0)               # rosso fisso
        self.lcd.custom_symbol(1, config.NOTE_GLYPH)
        self.lcd.custom_symbol(2, config.SPINA_GLYPH)
        self.seg7.reset()
        self.pres.show_welcome()
        time.sleep(3)                              # splash (come delay(3000))
        if self.storage.load(self.state):
            self.seg7.set_brightness(self.state.brightness)
        self.pres.render_full()
        print("=== R080 TIMER PRONTO ===")

    async def _input_loop(self):
        while True:
            self.main_btn.update()
            if self.state.is_main_btn_pressed and not self.state.is_action_started:
                self._start_action()
            elif (not self.state.is_main_btn_pressed) and self.state.is_action_started:
                self._stop_action()
            if not self.state.is_action_started:
                self.inputs.poll_encoders()
                self.inputs.poll_keypad()
            self.sound.tick()
            await asyncio.sleep_ms(5)

    async def _main(self):
        self.setup()
        await asyncio.gather(self._input_loop(), self.engine.run_render())

    def run(self):
        asyncio.run(self._main())
