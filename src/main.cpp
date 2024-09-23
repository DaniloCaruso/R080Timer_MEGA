#include <Arduino.h>
#include <Keypad.h>
#include <avr/eeprom.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <Encoder.h>
#include <EncoderButton.h>
#include "types.h"
#include "constants.h"
#include "variables.h"
#include "functions/base.h"
#include "functions/config.h"
#include "functions/presentation.h"
#include "functions/sound.h"
#include "functions/running_mgmt.h"
#include "functions/operations.h"
#include "functions/encoder_mgmt.h"
#include "functions/keypad_mgmt.h"

boolean isRunnninng = false;

void setup() {

  Serial.begin(9600);
  initializePinMode();
  configKeypad();
  configLCD();
  config7SegmentsDisplay();
  showWelcomeMessage();
  delay(2000);
  reset7SegmentsDisplay();
  resetLCD();
  showSound();
  // Serial.println("[SETUP 8] SOUND EMITTED");
  showChannel();
  // Serial.println("[SETUP 9] CHANNEL SHOWED");
  setErrlet();
  // Serial.println("[SETUP 10] ERRLET SETTED");
  digitalWrite(RELAY_PIN, HIGH); // RELE SPENTO
  // Serial.println("[SETUP 11] CLOSE RELE");
  initializeOperations();
  // Serial.println("[SETUP 12] OPERATIONS INITIALIZED");
  registerClickHandler();
  // Serial.println("[SETUP 13] CLICK HANDLER REGISTERED");
}

void loop() {

  if (isInError()) return;
  evaluateMainBtnStatus();
  manageMainButtonAction();
}

