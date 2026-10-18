// VARIABLES_H

#ifndef VARIABLES_H
#define VARIABLES_H

#include <Arduino.h>
#include <Keypad.h>
#include <SoftwareSerial.h>
#include <Encoder.h>
#include <OneButton.h>
#include "constants.h"
#include "Waveshare_LCD1602_RGB.h"
#include "functions/fstop.h"

// Caratteri personalizzati LCD
uint8_t note[8] = { 0x2, 0x3, 0x2, 0x2, 0xe, 0x1e, 0xc, 0x0 };
uint8_t spina[8] = { 0xa, 0xa, 0x1f, 0x11, 0x11, 0xe, 0x4, 0x4 };

// Stato globale
int brightness = 10;
char keyBuffer[17];
int timeSet = 0;
int appoTimeSet = 0;
byte nCifra = 0;
byte channel = 1;
boolean mute = false;
boolean channelsRelayActivated[3] = { true, true, true };
int channelTime[3] = { 0, 0, 0 };
FStopState fstopState = { 0, 0 };

volatile boolean isMainBtnPressed = false;
volatile boolean isActionStarted = false;
const unsigned long DEBOUNCE_TIME = 50;

// Motore temporale Timer1 ISR (vedi functions/timer_isr.h)
volatile uint32_t g_tickMs           = 0;  // contatore monotonico in ms
volatile uint32_t g_remainingMs      = 0;  // residuo countdown
volatile uint32_t g_elapsedMs        = 0;  // elapsed focus/test-strip
volatile uint32_t g_nextStepTargetMs = 0;  // target test-strip (ms assoluti)
volatile uint8_t  g_relayGate        = 0;  // snapshot canale attivo all'armo
volatile uint8_t  g_modeActive       = 0;  // 0=none, 1=focus, 2=countdown, 3=teststrip
volatile uint8_t  g_tickFlag100ms    = 0;  // set ogni 100 ISR (10 Hz)
volatile uint8_t  g_endOfCountdown   = 0;  // set quando remaining->0
volatile uint8_t  g_nextStepReached  = 0;  // set quando elapsed >= target step
volatile uint8_t  g_metronomeFlag    = 0;  // set ogni secondo in countdown

// Hardware
Keypad keypad = Keypad(makeKeymap(MATRIX_KEYS), MATRIX_ROWS_PIN, MATRIX_COLS_PIN, MATRIX_ROWS_N, MATRIX_COLS_N);
SoftwareSerial sevenSegmentsDisplay(1, DISPLAY_7_SEG_PIN);
Waveshare_LCD1602_RGB lcd(16,2);

Encoder encoderMode(ENC_MODE_DT_PIN, ENC_MODE_CLK_PIN);
Encoder encoderPrecision(ENC_PRECISION_DT_PIN, ENC_PRECISION_CLK_PIN);

OneButton encoderModeButton(ENC_MODE_SW_PIN, true);
OneButton encoderPrecisionButton(ENC_PRECISION_SW_PIN, true);

OneButton mainButton(MAIN_BTN_PIN, true);

Operation operations[3];
OperationStatus operationStatus;

#endif // VARIABLES_H
