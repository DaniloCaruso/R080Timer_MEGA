#include "Screen.h"

#include <string.h>
#include "Precision.h"

constexpr uint32_t SPLASH_MS = 1500;  // REQ-7.9
constexpr uint32_t WDT_MESSAGE_MS = 3000;  // REQ-9.9

// Scrive n senza zeri iniziali; ritorna le cifre scritte.
static uint8_t writeNumber(char* p, uint16_t n) {
  uint8_t len = n >= 1000 ? 4 : n >= 100 ? 3 : n >= 10 ? 2 : 1;
  for (uint8_t i = len; i > 0; i--, n /= 10) p[i - 1] = '0' + n % 10;
  return len;
}

void Screen::compose(uint32_t nowMs) {
  memset(want_, ' ', sizeof want_);
  if (watchdogReset_ && nowMs < WDT_MESSAGE_MS) {
    memcpy(want_[0], "RESET: WDT", 10);
    return;
  }
  if (nowMs < SPLASH_MS) {
    memcpy(want_[0], " - R080 Timer - ", 16);
    uint8_t n = strlen(version_);
    if (n > 15) n = 15;
    uint8_t col = (16 - n - 1) / 2;
    want_[1][col] = 'v';
    memcpy(&want_[1][col + 1], version_, n);
    return;
  }
  // Disposizione del firmware precedente. Riga 1: Modalità, tipo di precisione 12, Avanzamento 14.
  // Riga 2: precisione 0-8, Canale 9-12, nota 14, spina 15.
  // shortcut: testi in RAM perché il dominio non usa PROGMEM; da spostare se la RAM stringe.
  static const char kModeRows[MODE_COUNT][17] = {"FOCUS           ", "COUNTDOWN  [S]  ", "TEST STRIP [S]CO"};
  const ChannelRecord& c = ctl_.channel();
  memcpy(want_[0], kModeRows[c.mode], 16);
  if (c.mode == TEST_STRIP && c.advance) memcpy(&want_[0][14], "PP", 2);  // come il firmware precedente
  if (c.mode != FOCUS && c.precisionKind) want_[0][12] = 'F';
  uint8_t i = c.precisionIndex;
  if (c.mode == FOCUS) {
    // REQ-7.10: in Focus nessuna precisione.
  } else if (uint16_t n = ctl_.stripStep()) {
    memcpy(want_[1], "St:", 3);  // REQ-7.6: al posto di precisione e Offset
    writeNumber(&want_[1][3], n);
  } else if (c.precisionKind) {
    // F-Stop: precisione alle colonne 0-5 (`Pr1/64`), Offset alle 6-8 (REQ-4.4, TECHNICAL C-13).
    memcpy(want_[1], "Pr1/", 4);
    writeNumber(&want_[1][4], kFStopDenominators[i]);
    int8_t off = c.offset;
    want_[1][6] = off > 0 ? '+' : off < 0 ? '-' : ' ';
    if (off < 0) off = -off;
    want_[1][7] = '0' + off / 10;
    want_[1][8] = '0' + off % 10;
  } else {
    memcpy(want_[1], "Pr: ", 4);
    want_[1][4 + writeNumber(&want_[1][4], kSecondsSteps[i])] = 's';
  }
  memcpy(&want_[1][9], "Ch:", 3);
  want_[1][12] = 'A' + ctl_.channelIndex();
  want_[1][14] = ctl_.muted() ? ' ' : LCD_NOTE;
  want_[1][15] = c.relayEnabled ? LCD_PLUG : ' ';
}

void Screen::render(uint32_t nowMs) {
  if (!lcd_.online()) {
    if (online_) log_.line("LCD offline");  // REQ-2.6
    online_ = false;
    return;
  }
  if (!online_) {
    log_.line("LCD online");
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
