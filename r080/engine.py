import time
import uasyncio as asyncio
from machine import Timer
from r080 import config
from r080.stepmath import next_step_decis

MODE_NONE = 0
MODE_FOCUS = 1
MODE_COUNTDOWN = 2
MODE_TEST_STRIP = 3


class TimingEngine:
    """
    Motore temporale asincrono — rimpiazza Timer1 ISR 1kHz dell'originale Arduino.

    Callbacks iniettati dall'app:
      render(decis)            -> aggiorna LCD/7-seg
      on_metronome()           -> beep metronomo (countdown, ogni secondo)
      on_countdown_end()       -> cleanup fine countdown
      on_step(new_target)      -> beep step test-strip + UI
      on_step_warning()        -> beep di avviso (1..3 decis prima dello step)
      relay_off()              -> spegne relay (gate già valutato dall'app)
    """

    def __init__(self, render, on_metronome, on_countdown_end, on_step,
                 on_step_warning, relay_off):
        self.render = render
        self.on_metronome = on_metronome
        self.on_countdown_end = on_countdown_end
        self.on_step = on_step
        self.on_step_warning = on_step_warning
        self.relay_off = relay_off

        self.mode = MODE_NONE
        self.current_decis = 0
        self.next_step_target = 0
        self._is_fstop = False
        self._precision_value = 1
        self._modality_step = False

        self._end_ticks = 0
        self._last_second = -1
        self._step_start_ticks = 0
        self._step_base_decis = 0

        self._relay_timer = Timer(0)

    # --- avvio modalità (parità con start<mode>Timer) ---

    def start_focus(self):
        self.mode = MODE_FOCUS
        self.current_decis = 0

    def start_countdown(self, appo_decis, gate_enabled):
        """
        Avvia il countdown da appo_decis (decisecondi).
        gate_enabled: se True arma il one-shot Timer per relay_off preciso a fine corsa.
        """
        if appo_decis <= 0:
            return
        self.mode = MODE_COUNTDOWN
        self.current_decis = appo_decis
        self._end_ticks = time.ticks_add(time.ticks_ms(), appo_decis * 100)
        self._last_second = -1
        if gate_enabled:
            # one-shot per spegnimento relay preciso a fine corsa
            self._relay_timer.init(
                mode=Timer.ONE_SHOT,
                period=appo_decis * 100,
                callback=lambda t: self.relay_off(),
            )

    def start_test_strip(self, appo_decis, is_fstop, precision_value,
                         modality_step, resume):
        """
        Avvia (o riprende) il test-strip.
        resume=True: preserva current_decis e next_step_target (STEP_BY_STEP resume).
        """
        self._is_fstop = is_fstop
        self._precision_value = precision_value
        self._modality_step = modality_step
        self.mode = MODE_TEST_STRIP
        if not resume:
            self.current_decis = 0
            self.next_step_target = appo_decis if appo_decis > 0 else 10
        self._step_start_ticks = time.ticks_ms()
        self._step_base_decis = self.current_decis

    def stop(self):
        """Ferma tutti i timer e porta il motore in MODE_NONE."""
        self.mode = MODE_NONE
        self._relay_timer.deinit()

    # --- task asyncio (eseguito via asyncio.create_task) ---

    async def run_render(self):
        """Loop principale: tick ogni 100 ms, parità con g_tickFlag100ms dell'ISR."""
        while True:
            await asyncio.sleep_ms(100)
            self._tick_100ms()

    # --- logica tick (esposta per i test) ---

    def _tick_100ms(self):
        if self.mode == MODE_FOCUS:
            self.current_decis += 1
            self.render(self.current_decis)

        elif self.mode == MODE_COUNTDOWN:
            rem_ms = time.ticks_diff(self._end_ticks, time.ticks_ms())
            if rem_ms <= 0:
                # Fine countdown: relay_off lato software (il one-shot Timer
                # può aver già agito se gate_enabled era True, ma la chiamata
                # è idempotente).
                self.relay_off()
                self.mode = MODE_NONE
                self._relay_timer.deinit()
                self.render(0)
                self.on_countdown_end()
                return
            decis = rem_ms // 100
            self.current_decis = decis
            self.render(decis)
            # Metronomo: ogni cambio di secondo (parità g_metronomeFlag ISR)
            sec = rem_ms // 1000
            if sec != self._last_second:
                self._last_second = sec
                self.on_metronome()

        elif self.mode == MODE_TEST_STRIP:
            elapsed_ms = time.ticks_diff(time.ticks_ms(), self._step_start_ticks)
            self.current_decis = self._step_base_decis + elapsed_ms // 100
            self.render(self.current_decis)
            dist = self.next_step_target - self.current_decis
            if 0 < dist <= 3:
                self.on_step_warning()
            if self.current_decis >= self.next_step_target:
                self._advance_step()

    def _advance_step(self):
        """Calcola il prossimo target e gestisce CONTINOUS vs STEP_BY_STEP."""
        new_target = next_step_decis(
            self.current_decis, self._is_fstop, self._precision_value
        )
        self.next_step_target = new_target
        if self._modality_step:
            # STEP_BY_STEP: relay off + pausa (attende nuova pressione del bottone)
            self.relay_off()
            self.mode = MODE_NONE
        else:
            # CONTINOUS: ripartenza del contatore parziale dall'attuale posizione
            self._step_start_ticks = time.ticks_ms()
            self._step_base_decis = self.current_decis
        self.on_step(new_target)
