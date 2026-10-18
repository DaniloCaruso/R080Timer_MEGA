#include "ServiceMenu.h"

#include <stdio.h>
#include <string.h>
#include "ClockCorrection.h"

// Testi in flash (Flash.h): le righe si compongono con fsnprintf e putF.
// Le voci T_* sono i test hardware (REQ-9.5), contigue fra T_RELAY e T_MAIN_BUTTON.
enum : uint8_t { INFO, COUNTERS, HEALTH, T_RELAY, T_BUZZER, T_SEVEN_SEG, T_LCD, T_KEYPAD, T_ENCODERS,
                 T_MAIN_BUTTON, CALIBRATION, LCD_EXPOSURE, FOCUS_LIMIT, FACTORY, EXIT, ITEM_COUNT };
static const char kItems[ITEM_COUNT][16] PROGMEM = {"Info",           "Contatori",       "Salute",
                                            "Test rele",      "Test buzzer",     "Test 7 segmenti",
                                            "Test LCD",       "Test tastierino", "Test encoder",
                                            "Test pulsante",  "Calibrazione",    "LCD esposizione",
                                            "Tempo max Focus", "Ripristino fabb", "Esci"};
static const uint8_t kPages[ITEM_COUNT] = {1, 4, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1};  // schermate di ogni voce
static const char kCounterTitles[4][16] PROGMEM = {"Accensioni", "Reset watchdog", "Esposizioni", "Ore lampada"};
static const char kHealthTitles[3][16] PROGMEM = {"Loop max", "ISR max", "Tick persi"};
// REQ-9.6, REQ-9.12: le tre schermate della Calibrazione.
enum : uint8_t { CAL_LOG, CAL_PPM, CAL_MEASURE };
static const char kCalTitles[3][17] PROGMEM = {"Log calibrazione", "Correzione ppm", "Durata 600 s"};
constexpr uint32_t CAL_NOMINAL_MS = 600000;  // Countdown di riferimento di 600,0 s

void ServiceMenu::load() {
  typed_ = false;
  const GlobalRecord& g = set_.d.g;
  if (item_ == FOCUS_LIMIT) value_ = kFocusLimitMinutes[g.focusLimitIndex];
  else if (item_ == CALIBRATION) value_ = page_ == CAL_PPM ? g.clockPpm : CAL_NOMINAL_MS / 10;  // centesimi di s
  else value_ = g.lcdExposureLevel;
}

void ServiceMenu::turn(int8_t dir) {
  if (log_) return;  // il log di calibrazione si chiude solo con `D`
  if (!inside_) {
    item_ = (item_ + ITEM_COUNT + dir) % ITEM_COUNT;
    return;
  }
  page_ = (page_ + kPages[item_] + dir) % kPages[item_];
  load();
}

void ServiceMenu::click() {
  if (!inside_) {
    if (item_ == EXIT) {
      open_ = false;  // REQ-9.14: il timer riparte in uso normale con le impostazioni già salvate
      return;
    }
    inside_ = true;
    page_ = 0;
    memset(hw_, 0, sizeof hw_);
    load();
    return;
  }
  GlobalRecord& g = set_.d.g;
  if (item_ == CALIBRATION) {
    if (page_ == CAL_LOG) {
      log_ = true;
      return;
    }
    if (page_ == CAL_PPM ? value_ > PPM_LIMIT || value_ < -PPM_LIMIT : value_ <= 0) return refuse();
    // value_ in centesimi di secondo per la durata misurata
    g.clockPpm = page_ == CAL_PPM ? value_ : ppmFromMeasurement(CAL_NOMINAL_MS, value_ * 10, g.clockPpm);
  } else if (item_ == LCD_EXPOSURE) {  // REQ-9.7
    if (value_ < 1 || value_ > LEVEL_COUNT) return refuse();
    g.lcdExposureLevel = value_;
  } else if (item_ == FOCUS_LIMIT) {  // REQ-9.7, REQ-3.3
    uint8_t i = 0;
    while (i < FOCUS_LIMIT_COUNT && kFocusLimitMinutes[i] != value_) i++;
    if (i == FOCUS_LIMIT_COUNT) return refuse();
    g.focusLimitIndex = i;
  } else if (item_ == FACTORY) {  // REQ-9.8: la prima pressione chiede conferma, `D` annulla
    if (!page_) {
      page_ = 1;
      return;
    }
    set_.factoryReset();
  } else {
    return;
  }
  set_.changed();
  set_.save();
  sound_.play(Sound::Confirm);
  inside_ = false;
}

bool ServiceMenu::testing() const { return inside_ && item_ >= T_RELAY && item_ <= T_MAIN_BUTTON; }

bool ServiceMenu::relayOn() const { return inside_ && item_ == T_RELAY && hw_[0]; }

bool ServiceMenu::sevenSegTest() const { return inside_ && item_ == T_SEVEN_SEG; }

// Ogni test agisce solo sul suo componente.
void ServiceMenu::test(Input in, int8_t arg) {
  constexpr uint8_t SOUNDS = static_cast<uint8_t>(Sound::Confirm) + 1;
  int8_t& h = hw_[0];
  switch (item_) {
    case T_RELAY:  // hw_[0]: relè acceso
      if (in == Input::ModeClick) h = !h;
      break;
    case T_BUZZER:  // hw_[0]: suono scelto
      if (in == Input::ModeTurn) h = (h + SOUNDS + arg) % SOUNDS;
      else if (in == Input::ModeClick) sound_.play(static_cast<Sound>(h));
      break;
    case T_KEYPAD:  // hw_[0]: ultimo tasto; `D` si prova uscendo
      if (in == Input::Digit) h = '0' + arg;
      else if (in == Input::Channel) h = 'A' + arg;
      else if (in == Input::Star || in == Input::StarHold || in == Input::ZeroStar) h = '*';
      else if (in == Input::Hash || in == Input::HashHold || in == Input::ZeroHash) h = '#';
      break;
    case T_ENCODERS:  // posizione e clic: Modalità in hw_[0..1], precisione in hw_[2..3]
      if (in == Input::ModeTurn) hw_[0] += arg;
      else if (in == Input::ModeClick) hw_[1]++;
      else if (in == Input::PrecTurn) hw_[2] += arg;
      else if (in == Input::PrecClick) hw_[3]++;
      break;
    case T_MAIN_BUTTON:  // hw_[0]: 0 nessun evento, 1 premuto, 2 rilasciato
      if (in == Input::MainPress) h = 1;
      else if (in == Input::MainRelease) h = 2;
      break;
  }
}

// Valore non ammesso: suono di errore, la prossima cifra inizia un valore nuovo.
void ServiceMenu::refuse() {
  typed_ = false;
  sound_.play(Sound::Error);
}

void ServiceMenu::back() { inside_ = log_ = false; }

void ServiceMenu::digit(uint8_t d) {
  // A calcolatrice: due cifre per livelli e minuti, quattro per i ppm, cinque per i centesimi di secondo.
  int32_t keep = item_ != CALIBRATION ? 10 : page_ == CAL_PPM ? 1000 : 10000;
  if (!typed_) value_ = d;
  else value_ = value_ < 0 ? -(-value_ % keep * 10 + d) : value_ % keep * 10 + d;
  typed_ = true;
}

void ServiceMenu::negate() {
  value_ = -value_;
  typed_ = true;
}

// Copia s senza il terminatore, al massimo n caratteri: put da RAM, putF da flash.
static void put(char* row, const char* s, uint8_t n = 16) {
  for (uint8_t i = 0; i < n && s[i]; i++) row[i] = s[i];
}
static void putF(char* row, const char* s, uint8_t n = 16) {
  for (uint8_t i = 0; i < n && fbyte(s + i); i++) row[i] = fbyte(s + i);
}

void ServiceMenu::compose(char rows[2][16]) const {
  if (!inside_) {
    putF(rows[0], FSTR("MENU SERVIZIO"));
    rows[1][0] = '>';
    putF(&rows[1][1], kItems[item_], 15);
    return;
  }
  if (item_ == T_LCD) {  // REQ-9.5: blocco pieno in ogni cella
    memset(rows, 0xFF, 2 * 16);
    return;
  }
  char b[24];  // le righe oltre 16 caratteri (encoder) le taglia put()
  const CounterRecord& c = cnt_.d;
  const char* title = kItems[item_];
  switch (item_) {
    case INFO:  // REQ-9.2
      fsnprintf(b, sizeof b, FSTR("R080 v%s"), version_);
      put(rows[0], b);
      put(rows[1], date_);
      return;
    case COUNTERS:  // REQ-9.3
      title = kCounterTitles[page_];
      if (page_ == 3)
        fsnprintf(b, sizeof b, FSTR("%luh %02lum"), (unsigned long)(c.lampSeconds / 3600),
                 (unsigned long)(c.lampSeconds / 60 % 60));
      else
        fsnprintf(b, sizeof b, FSTR("%lu"), (unsigned long)(page_ == 0 ? c.powerOns : page_ == 1 ? c.watchdogResets : c.exposures));
      break;
    case HEALTH:  // REQ-9.4
      title = kHealthTitles[page_];
      if (page_ == 0) fsnprintf(b, sizeof b, FSTR("%lu us"), (unsigned long)health_.loopMaxUs);
      else if (page_ == 1) fsnprintf(b, sizeof b, FSTR("%lu us"), (unsigned long)health_.isrMaxUs);
      else fsnprintf(b, sizeof b, FSTR("%lu"), (unsigned long)health_.lostTicks);
      break;
    case CALIBRATION:
      title = kCalTitles[page_];
      if (page_ == CAL_LOG) fsnprintf(b, sizeof b, log_ ? FSTR("Attivo, D: esci") : FSTR("Clic: avvia"));
      else if (page_ == CAL_PPM) fsnprintf(b, sizeof b, FSTR("ppm: %+ld"), (long)value_);
      else fsnprintf(b, sizeof b, FSTR("%ld,%02ld s"), (long)(value_ / 100), (long)(value_ % 100));
      break;
    case LCD_EXPOSURE:
      fsnprintf(b, sizeof b, FSTR("Livello 1-8: %d"), int(value_));  // due cifre al massimo
      break;
    case FOCUS_LIMIT:
      fsnprintf(b, sizeof b, FSTR("Minuti: %ld"), (long)value_);
      break;
    case T_RELAY:
      fsnprintf(b, sizeof b, hw_[0] ? FSTR("Clic: acceso") : FSTR("Clic: spento"));
      break;
    case T_BUZZER:
      fsnprintf(b, sizeof b, FSTR("Suono %u: clic"), hw_[0] + 1);
      break;
    case T_SEVEN_SEG:
      fsnprintf(b, sizeof b, FSTR("Accesi: 888.8"));
      break;
    case T_KEYPAD:
      fsnprintf(b, sizeof b, FSTR("Tasto: %c"), hw_[0] ? hw_[0] : '-');
      break;
    case T_ENCODERS:
      fsnprintf(b, sizeof b, FSTR("M%+d c%u  P%+d c%u"), hw_[0], uint8_t(hw_[1]), hw_[2], uint8_t(hw_[3]));
      break;
    case T_MAIN_BUTTON:
      fsnprintf(b, sizeof b, hw_[0] == 1 ? FSTR("Premuto") : hw_[0] == 2 ? FSTR("Rilasciato") : FSTR("Premi"));
      break;
    case FACTORY:
      if (page_) title = FSTR("Confermi?");
      fsnprintf(b, sizeof b, page_ ? FSTR("clic=Si D=No") : FSTR("Clic: ripristina"));
      break;
    default:
      fsnprintf(b, sizeof b, FSTR("Clic: conferma"));
      break;
  }
  putF(rows[0], title);
  put(rows[1], b);
}
