#include "Settings.h"

#include <string.h>
#include "Crc8.h"
#include "Precision.h"

constexpr uint8_t VERSION = 1;
constexpr uint8_t HEADER = 4, BLOCK_LEN = HEADER + sizeof(SettingsData) + 1;  // 43 byte
constexpr uint32_t SAVE_DELAY_MS = 2000;  // REQ-6.7

// Valori di fabbrica (C-10): Canale A, Countdown, Secondi 1 s, continuo, 10,0 s, relè abilitato;
// mute spento, LCD livello 3, 7 segmenti livello 3, LCD in esposizione livello 1, Focus 5 min, 0 ppm.
constexpr ChannelRecord kFactoryChannel = {COUNTDOWN, 0, 0, 0, 100, 100, 0, 1};
constexpr SettingsData kFactory = {{0, 0, 3, 3, 1, 1, 0}, {kFactoryChannel, kFactoryChannel, kFactoryChannel}};

// Anche con il CRC giusto, valori che indicizzano tabelle restano nei limiti.
static bool inRange(const SettingsData& d) {
  if (d.g.activeChannel > 2) return false;
  for (const ChannelRecord& c : d.ch)
    if (c.mode >= MODE_COUNT || c.precisionIndex >= PRECISION_COUNT) return false;
  return true;
}

void Settings::load() {
  uint8_t b[BLOCK_LEN];
  storage_.read(0, b, BLOCK_LEN);
  memcpy(&d, b + HEADER, sizeof d);
  // REQ-2.5, REQ-6.6: il vecchio formato (0xA5 all'indirizzo 0) non passa il controllo.
  bool ok = b[0] == 'R' && b[1] == '8' && b[2] == VERSION && b[3] == BLOCK_LEN &&
            crc8(b, BLOCK_LEN - 1) == b[BLOCK_LEN - 1] && inRange(d);
  if (!ok) d = kFactory;
}

void Settings::service(uint32_t nowMs) {
  if (restart_) {
    sinceMs_ = nowMs;
    restart_ = false;
  }
  if (pending_ && nowMs - sinceMs_ >= SAVE_DELAY_MS) save();
}

// REQ-6.8: confronta con la EEPROM e passa a update() solo i byte cambiati.
void Settings::save() {
  if (!pending_) return;
  pending_ = false;
  uint8_t b[BLOCK_LEN] = {'R', '8', VERSION, BLOCK_LEN};
  memcpy(b + HEADER, &d, sizeof d);
  b[BLOCK_LEN - 1] = crc8(b, BLOCK_LEN - 1);
  for (uint8_t i = 0; i < BLOCK_LEN; i++) {
    uint8_t old;
    storage_.read(i, &old, 1);
    if (old != b[i]) storage_.update(i, &b[i], 1);
  }
}
