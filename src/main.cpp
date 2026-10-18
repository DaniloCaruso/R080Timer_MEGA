// R080 Timer: collega i driver al dominio e fa girare il loop, senza attese.
#include <Arduino.h>
#include <Controller.h>
#include "drivers/InputDevices.h"
#include "drivers/SevenSegSerial3.h"
#include "drivers/Timer1Clock.h"

static Timer1Clock timing;
static SevenSegSerial3 sevenSeg;
static Controller controller(timing, sevenSeg);
static InputDevices inputs(controller);

void setup() {
  timing.begin();  // per primo: relè spento (REQ-2.1)
  sevenSeg.begin();
  inputs.begin();
}

void loop() {
  inputs.poll();
  controller.update();
}
