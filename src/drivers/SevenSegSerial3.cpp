#include "SevenSegSerial3.h"

#include <Arduino.h>

void SevenSegSerial3::begin() {
  Serial3.begin(9600);
  Serial3.write(0x76);  // reset del display
  Serial3.write(0x7A);  // luminosità
  Serial3.write(10);    // shortcut: valore fisso di oggi (livello 3), i livelli arrivano con il ticket delle luci
}

void SevenSegSerial3::showDeciseconds(uint16_t ds) {
  char digits[5];
  snprintf(digits, sizeof digits, "%04u", ds);
  Serial3.write(0x77);
  Serial3.write(0b00000100);  // punto decimale prima dell'ultima cifra
  Serial3.write(digits, 4);
}
