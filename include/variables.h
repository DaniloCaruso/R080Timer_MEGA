// VARIABLES_H

#ifndef VARIABLES_H
#define VARIABLES_H

#include <Arduino.h>
#include <ArduinoSTL.h>
#include <map>
#include <Keypad.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Encoder.h>
#include <EncoderButton.h>
#include "constants.h"

// Caratteri personalizzati LCD
uint8_t note[8] = { 0x2, 0x3, 0x2, 0x2, 0xe, 0x1e, 0xc, 0x0 };
uint8_t spina[8] = { 0xa, 0xa, 0x1f, 0x11, 0x11, 0xe, 0x4, 0x4 };
uint8_t frecciasu[8] = { 0x4, 0xe, 0x1f, 0x4, 0x4, 0x4, 0x4, 0x4 };
uint8_t frecciagiu[8] = { 0x4, 0x4, 0x4, 0x4, 0x4, 0x1f, 0xe, 0x4 };

int brightness = 10;
char keyBuffer[17];
int countdownTime[4];
int testStripTime[4];
int timeSet = 0;
int lastTimeSet = 0;
int nextTimeSet = 0;
int appoTimeSet;
byte nCifra = 0;
float multSet = 1.0;
byte channel = 1;
boolean mute = false;
boolean channelsRelayActivated[4]; // ATTIVAZIONE RELE?
int channelsCountdownTime[4];
int channelsStripTestTime[4];
int pushOrPullActMode; // = LOW;  //Modalità di attivazione alla pressione o al rilascio del pedale/pulsante (LOW = Rilascio; HIGH = pressione)
long errlet = 0;
boolean isMainBtnPressed = false;
boolean isActionStarted = false;

std::map<String, int> mainBtnTemporalStatus = { 
  { "current", LOW }, 
  { "last",  LOW } 
};

std::map<String, boolean> mainBtnActionStatus = { 
  { "TO_STOP", false },  
  { "TO_START", false },
  { "STARTED",  false },
  { "STOPPED", true }
};

Keypad keypad = Keypad(makeKeymap(MATRIX_KEYS), MATRIX_ROWS_PIN, MATRIX_COLS_PIN, MATRIX_ROWS_N, MATRIX_COLS_N);
SoftwareSerial sevenSegmentsDisplay(1, DISPLAY_7_SEG_PIN);
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS_N, LCD_ROWS_N);

Encoder encoderMode(ENC_MODE_DT_PIN, ENC_MODE_CLK_PIN);
Encoder encoderPrecision(ENC_PRECISION_DT_PIN, ENC_PRECISION_CLK_PIN);

EncoderButton encoderModeButton = EncoderButton(ENC_MODE_SW_PIN);
EncoderButton encoderPrecisionButton = EncoderButton(ENC_PRECISION_SW_PIN);

Operation operations[3];
OperationStatus operationStatus; 

#endif // VARIABLES_H