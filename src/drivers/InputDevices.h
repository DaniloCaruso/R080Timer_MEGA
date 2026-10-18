// Tastierino e pulsante principale: traducono le pressioni in Input del Controller.
#pragma once

#include <Controller.h>

class InputDevices {
 public:
  explicit InputDevices(Controller& ctl) : ctl_(ctl) {}
  void begin();
  void poll();  // a ogni giro di loop

 private:
  Controller& ctl_;
};
