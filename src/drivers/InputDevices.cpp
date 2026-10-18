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
static OneButton modeButton;
static Encoder precEncoder(ENC_PREC_DT_PIN, ENC_PREC_CLK_PIN);
static Encoder modeEncoder(ENC_MODE_DT_PIN, ENC_MODE_CLK_PIN);
static int32_t precLast = 0, modeLast = 0;  // ultime posizioni già tradotte in tacche
static bool held[2];  // HOLD già scattato per `#` [0] e `*` [1]: Keypad manda sempre un RELEASED dopo l'HOLD
static bool zeroDown = false, zeroUsed = false;  // `0` premuto; già usato come cifra o in combinazione

// Un cambio di stato di un tasto (C-20). `*` e `#` agiscono al rilascio e solo se non è scattato
// l'HOLD; rilasciati con `0` premuto fanno la combinazione. `0` agisce al rilascio, per non
// inserire la cifra quando fa parte di una combinazione.
static void onKey(Controller& ctl, char key, KeyState s) {
  if (key == '*' || key == '#') {
    bool star = key == '*';
    if (s == HOLD && !zeroDown) {
      held[star] = true;
      ctl.onInput(star ? Input::StarHold : Input::HashHold, 0);
    } else if (s == RELEASED) {
      if (!held[star] && zeroDown) {
        zeroUsed = true;
        ctl.onInput(star ? Input::ZeroStar : Input::ZeroHash, 0);
      } else if (!held[star]) {
        ctl.onInput(star ? Input::Star : Input::Hash, 0);
      }
      held[star] = false;
    }
    return;
  }
  if (key == '0') {
    if (s == PRESSED) {
      zeroDown = true;
      zeroUsed = false;
    } else if (s == RELEASED) {
      zeroDown = false;
      if (!zeroUsed) ctl.onInput(Input::Digit, 0);
    }
    return;
  }
  if (s != PRESSED) return;
  if (zeroDown && !zeroUsed) {  // un'altra cifra prima del rilascio di `0`: lo `0` entra per primo
    zeroUsed = true;
    ctl.onInput(Input::Digit, 0);
  }
  if (key >= '1' && key <= '9') ctl.onInput(Input::Digit, key - '0');
  else if (key >= 'A' && key <= 'C') ctl.onInput(Input::Channel, key - 'A');
  else if (key == 'D') ctl.onInput(Input::KeyD, 0);
}

void InputDevices::begin() {
  keypad.setHoldTime(1000);  // REQ-5.5, REQ-5.6
  mainButton.setup(MAIN_BTN_PIN, INPUT_PULLUP, true);
  mainButton.setDebounceMs(20);
  // REQ-5.11: l'evento scatta alla pressione, appena superato l'antirimbalzo.
  mainButton.attachPress([](void* c) { static_cast<Controller*>(c)->onInput(Input::MainPress, 0); }, &ctl_);
  precButton.setup(ENC_PREC_SW_PIN, INPUT_PULLUP, true);
  precButton.setDebounceMs(20);
  precButton.attachClick([](void* c) { static_cast<Controller*>(c)->onInput(Input::PrecClick, 0); }, &ctl_);
  modeButton.setup(ENC_MODE_SW_PIN, INPUT_PULLUP, true);
  modeButton.setDebounceMs(20);
  modeButton.attachClick([](void* c) { static_cast<Controller*>(c)->onInput(Input::ModeClick, 0); }, &ctl_);
}

// Variazione relativa (Modalità e precisione correnti stanno nel Canale): una tacca ogni 4 conteggi.
static void turns(Controller& ctl, Encoder& enc, int32_t& last, Input in) {
  int32_t pos = enc.read();
  while (pos - last >= 4) { last += 4; ctl.onInput(in, 1); }
  while (last - pos >= 4) { last -= 4; ctl.onInput(in, -1); }
}

void InputDevices::poll() {
  mainButton.tick();
  precButton.tick();
  modeButton.tick();
  turns(ctl_, precEncoder, precLast, Input::PrecTurn);
  turns(ctl_, modeEncoder, modeLast, Input::ModeTurn);
  // Più tasti insieme (combinazione `0`+`*`/`#`): getKeys() e tutta la lista dei tasti, non getKey().
  if (keypad.getKeys())
    for (Key& k : keypad.key)
      if (k.stateChanged) onKey(ctl_, k.kchar, k.kstate);
}
