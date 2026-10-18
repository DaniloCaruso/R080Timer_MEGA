// R080 Timer: collega i driver al dominio e fa girare il loop, senza attese.
#include <Arduino.h>
#include <avr/wdt.h>
#include <Controller.h>
#include <Counters.h>
#include <Screen.h>
#include <ServiceMenu.h>
#include <Settings.h>
#include <SoundScheduler.h>
#include "drivers/BuzzerTone.h"
#include "drivers/EepromStorage.h"
#include "drivers/InputDevices.h"
#include "drivers/LcdWaveshare.h"
#include "drivers/SevenSegSerial3.h"
#include "drivers/SystemGuard.h"
#include "drivers/Timer1Clock.h"
#include "drivers/UsbLog.h"

static Timer1Clock timing;
static SevenSegSerial3 sevenSeg;
static LcdWaveshare lcd;
static UsbLog usbLog;
static BuzzerTone buzzer;
static SoundScheduler sound(buzzer);
static EepromStorage eeprom;
static Settings settings(eeprom);
static Counters counters(eeprom);
static Health health;
static ServiceMenu menu(settings, counters, health, sound, FW_VERSION, __DATE__);
static Controller controller(timing, sevenSeg, sound, settings, counters, usbLog, menu);
static Screen screen(lcd, usbLog, controller, FW_VERSION);
static InputDevices inputs(controller);

void setup() {
  timing.begin();  // per primo: relè spento (REQ-2.1)
  bool watchdogReset = SystemGuard::begin();  // REQ-2.2; da qui setup() deve durare meno di 250 ms
  bool keyD = InputDevices::keyDHeld();
  bool settingsOk = settings.load();  // REQ-6.4
  counters.boot(watchdogReset);
  if (watchdogReset) screen.showWatchdogReset();  // REQ-9.9
  usbLog.begin();
  controller.boot(watchdogReset, settingsOk, keyD);
  sevenSeg.begin();
  lcd.begin();
  inputs.begin();
}

void loop() {
  uint32_t startUs = micros();
  wdt_reset();
  inputs.poll();
  controller.update();
  uint32_t now = millis();
  sound.tick(now);
  screen.render(now);
  settings.service(now);
  counters.service();
  timing.measure(health);
  uint32_t us = micros() - startUs;  // REQ-9.4
  if (us > health.loopMaxUs) health.loopMaxUs = us;
}
