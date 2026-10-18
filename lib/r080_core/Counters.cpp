#include "Counters.h"

#include <string.h>
#include "Crc8.h"

constexpr uint16_t ADDR = 0x040;
constexpr uint8_t VERSION = 1;
constexpr uint8_t HEADER = 4, BLOCK_LEN = HEADER + sizeof(CounterRecord) + 1;  // 17 byte

// Un blocco non valido riparte da zero: i contatori servono solo alla diagnostica.
void Counters::boot(bool watchdogReset) {
  uint8_t b[BLOCK_LEN];
  storage_.read(ADDR, b, BLOCK_LEN);
  memcpy(&d, b + HEADER, sizeof d);
  bool ok = b[0] == 'R' && b[1] == 'C' && b[2] == VERSION && b[3] == BLOCK_LEN &&
            crc8(b, BLOCK_LEN - 1) == b[BLOCK_LEN - 1];
  if (!ok) d = {};
  // Un reset da watchdog non è un'accensione: la somma dei due contatori dà gli avvii.
  if (watchdogReset) d.watchdogResets++;
  else d.powerOns++;
  b[0] = 'R', b[1] = 'C', b[2] = VERSION, b[3] = BLOCK_LEN;
  memcpy(b + HEADER, &d, sizeof d);
  b[BLOCK_LEN - 1] = crc8(b, BLOCK_LEN - 1);
  storage_.update(ADDR, b, BLOCK_LEN);  // eeprom_update_block riscrive solo le celle cambiate
}
