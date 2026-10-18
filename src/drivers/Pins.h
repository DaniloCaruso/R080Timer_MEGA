// Pin della scheda: stessi collegamenti del firmware precedente, tranne il
// 7 segmenti che passa da A2 a D14 (TX3, vedi SevenSegSerial3).
#pragma once

#include <stdint.h>

// Relè ingranditore: D47 = PORTL bit PL2 (vedi Timer1Clock).
constexpr uint8_t MAIN_BTN_PIN = 2;
// Encoder della precisione: DT e CLK su INT3/INT2, pulsante su D16.
constexpr uint8_t ENC_PREC_DT_PIN = 18, ENC_PREC_CLK_PIN = 19, ENC_PREC_SW_PIN = 16;
constexpr uint8_t KEYPAD_ROW_PINS[4] = {33, 31, 29, 27};
constexpr uint8_t KEYPAD_COL_PINS[4] = {41, 39, 37, 35};
