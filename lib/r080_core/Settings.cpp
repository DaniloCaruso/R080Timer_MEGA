#include "Settings.h"

#include <string.h>
#include "Crc8.h"
#include "ClockCorrection.h"
#include "Precision.h"
#include "TimeUnits.h"

constexpr uint8_t VERSION = 1;
constexpr uint8_t HEADER = 4, BLOCK_LEN = HEADER + sizeof(SettingsData) + 1;  // 43 byte
constexpr uint32_t SAVE_DELAY_MS = 2000;  // REQ-6.7

// Valori di fabbrica (C-10): Canale A, Countdown, Secondi 1 s, continuo, 10,0 s, relè abilitato;
// mute spento, LCD livello 3, 7 segmenti livello 3, LCD in esposizione livello 1, Focus 5 min, 0 ppm.
constexpr ChannelRecord kFactoryChannel = {COUNTDOWN, 0, 0, 0, 100, 100, 0, 1};
constexpr SettingsData kFactory = {{0, 0, 3, 3, 1, 1, 0}, {kFactoryChannel, kFactoryChannel, kFactoryChannel}};

// Anche con il CRC giusto, indici di tabelle e valori restano nei limiti.
static bool inRange(const SettingsData& d) {
  if (d.g.activeChannel > 2 || d.g.focusLimitIndex >= FOCUS_LIMIT_COUNT) return false;
  if (d.g.clockPpm > PPM_LIMIT || d.g.clockPpm < -PPM_LIMIT) return false;  // REQ-1.7
  auto badLevel = [](uint8_t l) { return l < 1 || l > LEVEL_COUNT; };
  if (badLevel(d.g.lcdLevel) || badLevel(d.g.segLevel) || badLevel(d.g.lcdExposureLevel)) return false;
  for (const ChannelRecord& c : d.ch)
    if (c.mode >= MODE_COUNT || c.precisionIndex >= PRECISION_COUNT || c.timeDs > DS_MAX || c.baseDs > DS_MAX ||
        c.offset > 99 || c.offset < -99)
      return false;
  return true;
}

bool Settings::load() {
  uint8_t b[BLOCK_LEN];
  storage_.read(0, b, BLOCK_LEN);
  memcpy(&d, b + HEADER, sizeof d);
  // REQ-2.5, REQ-6.6: il vecchio formato (0xA5 all'indirizzo 0) non passa il controllo.
  bool ok = b[0] == 'R' && b[1] == '8' && b[2] == VERSION && b[3] == BLOCK_LEN &&
            crc8(b, BLOCK_LEN - 1) == b[BLOCK_LEN - 1] && inRange(d);
  if (!ok) d = kFactory;
  return ok;
}

void Settings::factoryReset() { d = kFactory; }

void Settings::service(uint32_t nowMs) {
  if (restart_) {
    sinceMs_ = nowMs;
    restart_ = false;
  }
  if (pending_ && nowMs - sinceMs_ >= SAVE_DELAY_MS) save();
  if (!writing_) return;
  // REQ-6.8: un byte cambiato per giro, dall'immagine attuale: una modifica durante il salvataggio
  // entra nello stesso salvataggio.
  uint8_t b[BLOCK_LEN] = {'R', '8', VERSION, BLOCK_LEN};
  memcpy(b + HEADER, &d, sizeof d);
  b[BLOCK_LEN - 1] = crc8(b, BLOCK_LEN - 1);
  writing_ = !writeBlockStep(storage_, 0, b, BLOCK_LEN);
}

void Settings::save() {
  if (!pending_) return;
  pending_ = false;
  writing_ = true;
}
