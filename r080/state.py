from r080.fstop import FStopState


class AppState:
    def __init__(self):
        self.brightness = 10            # 1..10
        self.mute = False
        self.time_set = 0               # decisecondi correnti (7-seg)
        self.appo_time_set = 0          # backup per repeat-last-exposure
        self.n_cifra = 0                # cifre keypad in corso (0..3)
        self.channel = 1                # 1..3
        self.channel_time = [0, 0, 0]   # memoria per canale (decis)
        self.channels_relay_activated = [True, True, True]
        self.fstop = FStopState()
        self.is_main_btn_pressed = False
        self.is_action_started = False

    def channel_index(self):
        return self.channel - 1
