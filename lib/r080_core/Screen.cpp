#include "Screen.h"

#include <string.h>

constexpr uint32_t SPLASH_MS = 1500;  // REQ-7.9

void Screen::compose(uint32_t nowMs) {
  memset(want_, ' ', sizeof want_);
  if (nowMs < SPLASH_MS) {
    memcpy(want_[0], " - R080 Timer - ", 16);
    uint8_t n = strlen(version_);
    if (n > 15) n = 15;
    uint8_t col = (16 - n - 1) / 2;
    want_[1][col] = 'v';
    memcpy(&want_[1][col + 1], version_, n);
    return;
  }
  // Disposizione del firmware precedente. Riga 2: precisione 0-7, Canale 9-12, nota 14, spina 15.
  // shortcut: valori di fabbrica fissi (Countdown, Secondi 1 s, Canale A, mute spento, relè
  // abilitato) e testi in RAM perché il dominio non usa PROGMEM; diventano stato del Controller
  // con i ticket di suoni, precisione e canali.
  memcpy(want_[0], "COUNTDOWN  [S]  ", 16);
  memcpy(want_[1], "Pr: 1s   Ch:A ", 14);
  want_[1][14] = LCD_NOTE;
  want_[1][15] = LCD_PLUG;
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
