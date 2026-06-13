# --- Pin (ESP32-WROOM-32 38-pin) ---
I2C_SDA = 21
I2C_SCL = 22
SEG7_TX = 17
RELAY_PIN = 23
BUZZER_PIN = 19
MAIN_BTN_PIN = 5
ENC_MODE_CLK, ENC_MODE_DT, ENC_MODE_SW = 16, 4, 18
ENC_PREC_CLK, ENC_PREC_DT, ENC_PREC_SW = 25, 26, 27
KEYPAD_ROW_PINS = (13, 14, 32, 33)   # output, pilotate
KEYPAD_COL_PINS = (34, 35, 36, 39)   # input, pull-up esterni 10k

# --- I2C ---
LCD_ADDR = 0x3E
RGB_ADDR = 0x60

# --- Toni (Hz) ---
TONE_UP = 600
TONE_DOWN = 300

# --- Keymap 4x4 ---
KEYMAP = (
    ("1", "2", "3", "A"),
    ("4", "5", "6", "B"),
    ("7", "8", "9", "C"),
    ("*", "0", "#", "D"),
)

# --- Testi modalità ---
MODE_TEXTS = ("FOCUS", "COUNTDOWN", "TEST STRIP")

# --- Modalità / precisione (enum come interi) ---
FOCUS, COUNTDOWN, TEST_STRIP = 0, 1, 2
CONTINOUS, STEP_BY_STEP, NO_MODALITY = 0, 1, 2   # "CONTINOUS" voluto (parità originale)
FSTOP, SECONDS, NO_PRECISION = 0, 1, 2

PRECISION_COUNT = 11
# (valore, etichetta) — denominatori F-stop
FSTOP_PRECISIONS = (
    (1, "1/1"), (2, "1/2"), (3, "1/3"), (4, "1/4"), (6, "1/6"), (8, "1/8"),
    (12, "1/12"), (16, "1/16"), (24, "1/24"), (32, "1/32"), (64, "1/64"),
)
# (valore secondi, etichetta)
SECONDS_PRECISIONS = (
    (1, "1s"), (2, "2s"), (3, "3s"), (4, "4s"), (5, "5s"), (6, "6s"),
    (7, "7s"), (8, "8s"), (10, "10s"), (15, "15s"), (20, "20s"),
)

# --- Limiti tempo (decisecondi) ---
TIME_MIN = 0
TIME_MAX = 9999

# --- Glifi LCD custom ---
NOTE_GLYPH = (0x2, 0x3, 0x2, 0x2, 0xE, 0x1E, 0xC, 0x0)
SPINA_GLYPH = (0xA, 0xA, 0x1F, 0x11, 0x11, 0xE, 0x4, 0x4)

# --- Persistenza ---
NVS_NAMESPACE = "r080"
NVS_VERSION = 1
