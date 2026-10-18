#include "SevenSegSerial3.h"

#include <Arduino.h>

void SevenSegSerial3::begin() {
  Serial3.begin(9600);
  Serial3.write(0x76);  // reset del display
}

// REQ-7.11: livelli 1..8, più fitti in basso; il livello 3 è il valore 10 della versione precedente.
static constexpr uint8_t kSegLevels[LEVEL_COUNT] = {2, 5, 10, 20, 40, 80, 150, 255};

void SevenSegSerial3::setBrightness(uint8_t level) {
  Serial3.write(0x7A);
  Serial3.write(kSegLevels[level - 1]);
}

// Quattro cifre con il punto dopo la cifra point (bit 0 = dopo la prima).
static void show(uint16_t n, uint8_t point) {
  char digits[5];
  snprintf_P(digits, sizeof digits, PSTR("%04u"), n);
  Serial3.write(0x77);
  Serial3.write(point);
  Serial3.write(digits, 4);
}

void SevenSegSerial3::showDeciseconds(uint16_t ds) { show(ds, 0b00000100); }  // `999.9`

void SevenSegSerial3::showMinSec(uint16_t mmss) { show(mmss, 0b00000010); }  // `17.05`
