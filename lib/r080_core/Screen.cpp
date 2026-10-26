#include "Screen.h"

#include <string.h>
#include "Precision.h"
#include "Texts.h"

constexpr uint32_t SPLASH_MS = 1500;  // REQ-7.9
constexpr uint32_t WDT_MESSAGE_MS = 3000;  // REQ-9.9

// Copia il testo id nella lingua lang, senza il terminatore.
static void putText(char* p, TextId id, uint8_t lang) {
  for (const char* s = text(id, lang); fbyte(s); s++) *p++ = fbyte(s);
}

// Scrive n senza zeri iniziali; ritorna le cifre scritte.
static uint8_t writeNumber(char* p, uint16_t n) {
  uint8_t len = n >= 1000 ? 4 : n >= 100 ? 3 : n >= 10 ? 2 : 1;
  for (uint8_t i = len; i > 0; i--, n /= 10) p[i - 1] = '0' + n % 10;
  return len;
}

void Screen::compose(uint32_t nowMs) {
  memset(want_, ' ', sizeof want_);
  uint8_t lang = ctl_.language();
  if (watchdogReset_ && nowMs < WDT_MESSAGE_MS) {
    putText(want_[0], T_WDT_RESET, lang);
    return;
  }
  if (nowMs < SPLASH_MS) {
    fmemcpy(want_[0], FSTR(" - R080 Timer - "), 16);
    uint8_t n = strlen(version_);
    if (n > 15) n = 15;
    uint8_t col = (16 - n - 1) / 2;
    want_[1][col] = 'v';
    memcpy(&want_[1][col + 1], version_, n);
    return;
  }
  if (const ServiceMenu* menu = ctl_.service()) return menu->compose(want_);
  // Disposizione del firmware precedente. Riga 1: Modalità, tipo di precisione 12, Avanzamento 14.
  // Riga 2: precisione 0-6 senza prefisso, Canale 9-12, nota 14, spina 15.
  static const char kModeRows[MODE_COUNT][17] PROGMEM = {"FOCUS           ", "COUNTDOWN  [S]  ", "TEST STRIP [S]  "};
  const ChannelRecord& c = ctl_.channel();
  fmemcpy(want_[0], kModeRows[c.mode], 16);
  if (c.mode == TEST_STRIP) putText(&want_[0][14], c.advance ? T_ADV_STEP : T_ADV_CONT, lang);
  if (c.mode != FOCUS && c.precisionKind) want_[0][12] = 'F';
  uint8_t i = c.precisionIndex;
  if (c.mode == FOCUS) {
    // REQ-7.10: in Focus nessuna precisione.
  } else if (uint16_t n = ctl_.stripStep()) {
    putText(want_[1], T_STEP, lang);  // REQ-7.6: al posto di precisione e Offset
    writeNumber(&want_[1][3], n);
  } else if (c.precisionKind) {
    // F-Stop: `1/p` alle colonne 0-3, Offset alle 4-6 (REQ-4.4).
    fmemcpy(want_[1], FSTR("1/"), 2);
    writeNumber(&want_[1][2], kFStopDenominators[i]);
    int8_t off = c.offset;
    want_[1][4] = off > 0 ? '+' : off < 0 ? '-' : ' ';
    if (off < 0) off = -off;
    want_[1][5] = '0' + off / 10;
    want_[1][6] = '0' + off % 10;
  } else {
    // Secondi allineati a destra nelle colonne 0-1, poi `s`.
    writeNumber(&want_[1][kSecondsSteps[i] < 10], kSecondsSteps[i]);
    want_[1][2] = 's';
  }
  putText(&want_[1][9], T_CHANNEL, lang);
  want_[1][12] = 'A' + ctl_.channelIndex();
  want_[1][14] = ctl_.muted() ? ' ' : LCD_NOTE;
  want_[1][15] = c.relayEnabled ? LCD_PLUG : ' ';
}

void Screen::render(uint32_t nowMs) {
  if (!lcd_.online()) {
    if (online_) log_.print(FSTR("LCD offline"));  // REQ-2.6
    online_ = false;
    return;
  }
  if (!online_) {
    log_.print(FSTR("LCD online"));
    memset(shown_, 0, sizeof shown_);  // il driver l'ha reinizializzato: ridisegna tutto
    online_ = true;
  }
  uint8_t level = ctl_.lcdLevel();
  if (level != level_) lcd_.setBrightness(level_ = level);
  compose(nowMs);
  for (uint8_t r = 0; r < 2; r++) {
    uint8_t first = 0, last = 15;
    while (first < 16 && want_[r][first] == shown_[r][first]) first++;
    if (first == 16) continue;
    while (want_[r][last] == shown_[r][last]) last--;
    uint8_t n = last - first + 1;
    if (lcd_.writeAt(first, r, &want_[r][first], n)) memcpy(&shown_[r][first], &want_[r][first], n);
    return;  // al massimo una riga per giro: il loop resta breve
  }
}
