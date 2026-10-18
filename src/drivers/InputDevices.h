// Tastierino e pulsante principale: traducono le pressioni in Input del Controller.
#pragma once

#include <Controller.h>

class InputDevices {
 public:
  explicit InputDevices(Controller& ctl) : ctl_(ctl) {}
  static bool keyDHeld();  // in setup(), prima di begin(): `D` premuto all'accensione (REQ-9.1)
  void begin();
  void poll();  // a ogni giro di loop

 private:
  Controller& ctl_;
};
