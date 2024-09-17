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
  initializePinMode();
  configKeypad();
  configLCD();
  config7SegmentsDisplay();
  showWelcomeMessage();
  delay(2000);
  reset7SegmentsDisplay();
  resetLCD();
  showSound();
  showChannel();
  setErrlet();
  digitalWrite(RELAY_PIN, HIGH); // RELE SPENTO
  initializeOperations();
  registerClickHandler();
}

void loop() {

  if (isInError()) return;
  evaluateMainBtnStatus();
  manageMainButtonAction();
  
}

