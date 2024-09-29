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
#include <EncoderButton.h>

void setup() {

  Serial.begin(9600);
  digitalWrite(ENLARGER_RELAY_PIN, LOW); // RELE SPENTO
  initializePinMode();
  configKeypad();
  configLCD();
  config7SegmentsDisplay();
  showWelcomeMessage();
  delay(5000);
  reset7SegmentsDisplay();
  resetLCD();
  showSound();
  showChannel();
  setErrlet();
  initializeOperations();
  registerClickHandler();
  attachInterrupt(digitalPinToInterrupt(MAIN_BTN_PIN), mainButtonHandler, FALLING);
}

void loop() {

  
  manageMainButtonAction();
}
