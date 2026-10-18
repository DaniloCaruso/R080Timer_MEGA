// Canali e impostazioni globali, salvati in EEPROM in un blocco con versione e CRC8
// (TECHNICAL C-10): 'R','8', versione, lunghezza | GlobalRecord | ChannelRecord[3] | crc8.
#pragma once

#include <stdint.h>
#include "Hal.h"

enum Mode : uint8_t { FOCUS, COUNTDOWN, TEST_STRIP, MODE_COUNT };

// Un Canale: un'impostazione di stampa completa.
struct ChannelRecord {
  uint8_t mode, precisionKind, precisionIndex, advance;  // precisionKind 1 = F-Stop, advance 1 = passo-passo
  uint16_t timeDs, baseDs;
  int8_t offset;
  uint8_t relayEnabled;
};

struct GlobalRecord {
  uint8_t activeChannel, mute, lcdLevel, segLevel, lcdExposureLevel, focusLimitIndex;
  int16_t clockPpm;
};

// Tempo massimo del Focus, scelto con focusLimitIndex (REQ-3.3).
constexpr uint8_t kFocusLimitMinutes[] = {3, 5, 10, 15, 20};
constexpr uint8_t FOCUS_LIMIT_COUNT = sizeof kFocusLimitMinutes;

struct SettingsData {
  GlobalRecord g;
  ChannelRecord ch[3];
};
static_assert(sizeof(SettingsData) == 38, "layout EEPROM cambiato: aumentare la versione");

class Settings {
 public:
  explicit Settings(Storage& storage) : storage_(storage) {}
  bool load();                   // all'avvio: valori di fabbrica se il blocco non è valido (ritorna false)
  void factoryReset();           // REQ-9.8: tutte le impostazioni ai valori di fabbrica (poi changed())
  void changed() { pending_ = restart_ = true; }  // salva 2 s dopo l'ultima modifica
  void service(uint32_t nowMs);  // a ogni giro di loop
  void save();                   // subito, se c'è qualcosa da salvare (avvio di un'Esposizione)
  ChannelRecord& active() { return d.ch[d.g.activeChannel]; }
  const ChannelRecord& active() const { return d.ch[d.g.activeChannel]; }

  SettingsData d;

 private:
  Storage& storage_;
  bool pending_ = false, restart_ = false;
  uint32_t sinceMs_ = 0;
};
