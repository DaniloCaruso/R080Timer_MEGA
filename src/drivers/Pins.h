// Pin della scheda. Rispetto alla v1 il 7 segmenti passa da A2 a D14 (TX3, vedi
// SevenSegSerial3); dalla v2.1.0 ogni encoder ha i suoi tre fili su pin consecutivi.
#pragma once

#include <avr/io.h>
#include <stdint.h>

// Relè ingranditore: D47 = PORTL bit PL2. PORTL è fuori dallo spazio SBI/CBI e tone() scrive
// PL0 dalla sua ISR: dal main il relè si scrive solo in ATOMIC_BLOCK.
#define RELAY_ON() (PORTL |= _BV(PL2))
#define RELAY_OFF() (PORTL &= ~_BV(PL2))
constexpr uint8_t MAIN_BTN_PIN = 5;
constexpr uint8_t BUZZER_PIN = 49;  // PL0, tone() su Timer2
// Encoder della Modalità su D2-D4: CLK su INT4, DT su INT5, pulsante su D4.
constexpr uint8_t ENC_MODE_DT_PIN = 3, ENC_MODE_CLK_PIN = 2, ENC_MODE_SW_PIN = 4;
// Encoder della precisione su D17-D19: DT e CLK su INT3/INT2, pulsante su D17.
constexpr uint8_t ENC_PREC_DT_PIN = 18, ENC_PREC_CLK_PIN = 19, ENC_PREC_SW_PIN = 17;
constexpr uint8_t KEYPAD_ROW_PINS[4] = {33, 31, 29, 27};
constexpr uint8_t KEYPAD_COL_PINS[4] = {41, 39, 37, 35};
