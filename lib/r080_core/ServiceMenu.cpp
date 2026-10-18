#include "ServiceMenu.h"

#include <stdio.h>

// shortcut: mancano le voci Test hardware (issue #13) e Calibrazione (issue #14); entrano in questa tabella.
// shortcut: testi in RAM (circa 220 byte) come in Screen; da spostare in PROGMEM se la RAM stringe.
enum : uint8_t { INFO, COUNTERS, HEALTH, LCD_EXPOSURE, FOCUS_LIMIT, FACTORY, EXIT, ITEM_COUNT };
static const char kItems[ITEM_COUNT][16] = {"Info",          "Contatori",       "Salute", "LCD esposizione",
                                            "Tempo max Focus", "Ripristino fabb", "Esci"};
static const uint8_t kPages[ITEM_COUNT] = {1, 4, 3, 1, 1, 1, 1};  // schermate di ogni voce
static const char kCounterTitles[4][16] = {"Accensioni", "Reset watchdog", "Esposizioni", "Ore lampada"};
static const char kHealthTitles[3][16] = {"Loop max", "ISR max", "Tick persi"};

void ServiceMenu::turn(int8_t dir) {
  if (inside_) page_ = (page_ + kPages[item_] + dir) % kPages[item_];
  else item_ = (item_ + ITEM_COUNT + dir) % ITEM_COUNT;
}

void ServiceMenu::click() {
  if (!inside_) {
    if (item_ == EXIT) {
      open_ = false;  // REQ-9.14: il timer riparte in uso normale con le impostazioni già salvate
      return;
    }
    inside_ = true;
    page_ = 0;
    typed_ = false;
    value_ = item_ == FOCUS_LIMIT ? kFocusLimitMinutes[set_.d.g.focusLimitIndex] : set_.d.g.lcdExposureLevel;
    return;
  }
  GlobalRecord& g = set_.d.g;
  if (item_ == LCD_EXPOSURE) {  // REQ-9.7
    if (value_ < 1 || value_ > LEVEL_COUNT) return refuse();
    g.lcdExposureLevel = value_;
  } else if (item_ == FOCUS_LIMIT) {  // REQ-9.7, REQ-3.3
    uint8_t i = 0;
    while (i < FOCUS_LIMIT_COUNT && kFocusLimitMinutes[i] != value_) i++;
    if (i == FOCUS_LIMIT_COUNT) return refuse();
    g.focusLimitIndex = i;
  } else if (item_ == FACTORY) {  // REQ-9.8
    set_.factoryReset();
  } else {
    return;
  }
  set_.changed();
  set_.save();
  sound_.play(Sound::Confirm);
  inside_ = false;
}

// Valore non ammesso: suono di errore, la prossima cifra inizia un valore nuovo.
void ServiceMenu::refuse() {
  typed_ = false;
  sound_.play(Sound::Error);
}

void ServiceMenu::back() { inside_ = false; }

void ServiceMenu::digit(uint8_t d) {
  // A calcolatrice, due cifre: bastano per i livelli 1-8 e per i minuti del Focus.
  value_ = typed_ ? (value_ % 10) * 10 + d : d;
  typed_ = true;
}

// Copia s senza il terminatore, al massimo n caratteri.
static void put(char* row, const char* s, uint8_t n = 16) {
  for (uint8_t i = 0; i < n && s[i]; i++) row[i] = s[i];
}

void ServiceMenu::compose(char rows[2][16]) const {
  if (!inside_) {
    put(rows[0], "MENU SERVIZIO");
    rows[1][0] = '>';
    put(&rows[1][1], kItems[item_], 15);
    return;
  }
  char b[17];
  const CounterRecord& c = cnt_.d;
  const char* title = kItems[item_];
  switch (item_) {
    case INFO:  // REQ-9.2
      snprintf(b, sizeof b, "R080 v%s", version_);
      put(rows[0], b);
      put(rows[1], date_);
      return;
    case COUNTERS:  // REQ-9.3
      title = kCounterTitles[page_];
      if (page_ == 3)
        snprintf(b, sizeof b, "%luh %02lum", (unsigned long)(c.lampSeconds / 3600),
                 (unsigned long)(c.lampSeconds / 60 % 60));
      else
        snprintf(b, sizeof b, "%lu", (unsigned long)(page_ == 0 ? c.powerOns : page_ == 1 ? c.watchdogResets : c.exposures));
      break;
    case HEALTH:  // REQ-9.4
      title = kHealthTitles[page_];
      if (page_ == 0) snprintf(b, sizeof b, "%lu us", (unsigned long)health_.loopMaxUs);
      else if (page_ == 1) snprintf(b, sizeof b, "%lu us", (unsigned long)health_.isrMaxUs);
      else snprintf(b, sizeof b, "%lu", (unsigned long)health_.lostTicks);
      break;
    case LCD_EXPOSURE:
      snprintf(b, sizeof b, "Livello 1-8: %u", value_);
      break;
    case FOCUS_LIMIT:
      snprintf(b, sizeof b, "Minuti: %u", value_);
      break;
    default:
      snprintf(b, sizeof b, "Clic: conferma");
      break;
  }
  put(rows[0], title);
  put(rows[1], b);
}
