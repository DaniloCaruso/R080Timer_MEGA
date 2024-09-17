// CONSTANTS_H

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>
#include <ArduinoSTL.h>

const byte BUZZER_PIN = 10;
const byte RELAY_PIN = 11;
const byte MAIN_BTN_PIN = 12;
const byte DISPLAY_7_SEG_PIN = A2;

const uint8_t LCD_ADDRESS = 0x27;
const uint8_t LCD_ROWS_N = 2;
const uint8_t LCD_COLS_N = 16;

const byte ENC_MODE_DT_PIN = 2;
const byte ENC_MODE_CLK_PIN = 3;
const byte ENC_MODE_SW_PIN = 16;
const byte ENC_PRECISION_DT_PIN = 18;
const byte ENC_PRECISION_CLK_PIN = 19;
const byte ENC_PRECISION_SW_PIN = 17;

const byte MATRIX_ROWS_N = 4;
const byte MATRIX_COLS_N = 4;

char MATRIX_KEYS[MATRIX_ROWS_N][MATRIX_COLS_N] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

const String CHANNELS_STR[] = { "Ch:A", "Ch:B", "Ch:C" };

byte MATRIX_ROWS_PIN[MATRIX_ROWS_N] = { 31, 33, 35, 37 };
byte MATRIX_COLS_PIN[MATRIX_COLS_N] = { 39, 41, 43, 45 };

const int TONE_UP = 600;
const int TONE_DOWN = 300;

const String MODES_TEXTS[] = {
  "FOCUS",
  "COUNTDOWN",
  "TEST STRIP"
};


#endif // CONSTANTS_H