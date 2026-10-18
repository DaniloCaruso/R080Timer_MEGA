#include "InputDevices.h"

#include <Encoder.h>
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
static OneButton precButton;
static Encoder precEncoder(ENC_PREC_DT_PIN, ENC_PREC_CLK_PIN);
static int32_t precLast = 0;  // ultima posizione già tradotta in tacche
static Controller* keyTarget;  // il listener di Keypad non riceve contesto
static bool starHeld = false;

void InputDevices::begin() {
  keypad.setHoldTime(1000);  // REQ-5.5
  keyTarget = &ctl_;
  // Il listener riceve i cambi di stato del primo tasto premuto. `*` agisce al rilascio, e
  // solo se non è scattato l'HOLD: Keypad manda sempre un RELEASED dopo l'HOLD (C-20).
  keypad.addEventListener([](char key) {
    if (key != '*') return;
    KeyState s = keypad.getState();
    if (s == HOLD) {
      starHeld = true;
      keyTarget->onInput(Input::StarHold, 0);
    } else if (s == RELEASED) {
      if (!starHeld) keyTarget->onInput(Input::Star, 0);
      starHeld = false;
    }
  });
  mainButton.setup(MAIN_BTN_PIN, INPUT_PULLUP, true);
  mainButton.setDebounceMs(20);
  // REQ-5.11: l'evento scatta alla pressione, appena superato l'antirimbalzo.
  mainButton.attachPress([](void* c) { static_cast<Controller*>(c)->onInput(Input::MainPress, 0); }, &ctl_);
  precButton.setup(ENC_PREC_SW_PIN, INPUT_PULLUP, true);
  precButton.setDebounceMs(20);
  precButton.attachClick([](void* c) { static_cast<Controller*>(c)->onInput(Input::PrecClick, 0); }, &ctl_);
}

void InputDevices::poll() {
  mainButton.tick();
  precButton.tick();
  // Variazione relativa (la precisione corrente sta nel Controller): una tacca ogni 4 conteggi.
  int32_t pos = precEncoder.read();
  while (pos - precLast >= 4) { precLast += 4; ctl_.onInput(Input::PrecTurn, 1); }
  while (precLast - pos >= 4) { precLast -= 4; ctl_.onInput(Input::PrecTurn, -1); }
  char key = keypad.getKey();
  if (key >= '0' && key <= '9') ctl_.onInput(Input::Digit, key - '0');
  // shortcut: `#` agisce alla pressione finché non ha l'HOLD (Abilitazione relè), poi va al rilascio come `*`.
  else if (key == '#') ctl_.onInput(Input::Hash, 0);
}
