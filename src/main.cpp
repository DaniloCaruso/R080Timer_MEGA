#include <Arduino.h>
#include <Keypad.h>
#include <avr/eeprom.h>
#include <util/atomic.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <Encoder.h>
#include <OneButton.h>
#include "types.h"
#include "constants.h"
#include "variables.h"
#include "functions/timer_isr.h"
#include "functions/base.h"
#include "functions/config.h"
#include "functions/presentation.h"
#include "functions/sound.h"
#include "functions/storage.h"
#include "functions/running_mgmt.h"
#include "functions/operations.h"
#include "functions/encoder_mgmt.h"
#include "functions/keypad_mgmt.h"
#include "functions/timer_manager.h"

void setup() {
  Serial.begin(9600);
  digitalWrite(ENLARGER_RELAY_PIN, LOW);
  initializePinMode();
  configKeypad();
  configLCD();
  config7SegmentsDisplay();
  showWelcomeMessage();
  delay(3000);

  // Carica impostazioni EEPROM
  bool settingsLoaded = loadSettings();
  if (settingsLoaded) {
    config7SegmentsDisplay(); // applica brightness caricata
  }

  reset7SegmentsDisplay();
  resetLCD();
  initializeOperations();
  registerClickHandler();
  configureMainButton();

  // Mostra stato iniziale
  showSound();
  showChannel();
  showTime(timeSet);

  setupTimer1(); // motore temporale hardware — ultima istruzione prima del banner

  Serial.println(F("=== R080 TIMER PRONTO ==="));
}

void loop() {
  mainButton.tick();

  uint8_t flag100;
  uint8_t flagEnd;
  uint8_t flagStep;
  uint8_t flagMetro;

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    flag100   = g_tickFlag100ms;   g_tickFlag100ms   = 0;
    flagEnd   = g_endOfCountdown;  g_endOfCountdown  = 0;
    flagStep  = g_nextStepReached; g_nextStepReached = 0;
    flagMetro = g_metronomeFlag;   g_metronomeFlag   = 0;
  }

  if (flagEnd)   { onCountdownEnd(); }
  if (flagStep)  { onTestStripStep(); }
  if (flagMetro) { onMetronomeBeat(); }
  if (flag100)   { onTick100ms(); }

  tickSound();
  manageMainButtonAction();
}
