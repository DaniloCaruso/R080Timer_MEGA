#include "InputDevices.h"

#include <Keypad.h>
#include <OneButton.h>
#include "Pins.h"

static char keymap[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'},
};

// Keypad chiede puntatori non const ma non scrive sui pin.
static Keypad keypad(makeKeymap(keymap), const_cast<uint8_t*>(KEYPAD_ROW_PINS),
                     const_cast<uint8_t*>(KEYPAD_COL_PINS), 4, 4);
static OneButton mainButton;

void InputDevices::begin() {
  mainButton.setup(MAIN_BTN_PIN, INPUT_PULLUP, true);
  mainButton.setDebounceMs(20);
  // REQ-5.11: l'evento scatta alla pressione, appena superato l'antirimbalzo.
  mainButton.attachPress([](void* c) { static_cast<Controller*>(c)->onInput(Input::MainPress, 0); }, &ctl_);
}

void InputDevices::poll() {
  mainButton.tick();
  char key = keypad.getKey();
  if (key >= '0' && key <= '9') ctl_.onInput(Input::Digit, key - '0');
}
