// R080 Timer: collega i driver al dominio e fa girare il loop, senza attese.
#include <Arduino.h>
#include <Controller.h>
#include <Screen.h>
#include "drivers/InputDevices.h"
#include "drivers/LcdWaveshare.h"
#include "drivers/SevenSegSerial3.h"
#include "drivers/Timer1Clock.h"
#include "drivers/UsbLog.h"

static Timer1Clock timing;
static SevenSegSerial3 sevenSeg;
static LcdWaveshare lcd;
static UsbLog usbLog;
static Controller controller(timing, sevenSeg);
static Screen screen(lcd, usbLog, controller, FW_VERSION);
static InputDevices inputs(controller);

void setup() {
  timing.begin();  // per primo: relè spento (REQ-2.1)
  usbLog.begin();
  sevenSeg.begin();
  lcd.begin();
  inputs.begin();
}

void loop() {
  inputs.poll();
  controller.update();
  screen.render(millis());
}
