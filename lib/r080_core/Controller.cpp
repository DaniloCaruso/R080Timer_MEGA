#include "Controller.h"
#include "TimeUnits.h"

void Controller::onInput(Input in, int8_t arg) {
  bool running = timing_.running();
  if (in == Input::MainPress) {
    if (running) timing_.cancel();
    else if (timeDs_ > 0) timing_.startCountdown(dsToMs(timeDs_));
    entering_ = false;
  } else if (!running) {  // REQ-5.9: durante l'Esposizione conta solo il pulsante principale
    // Inserimento a calcolatrice: la cifra entra da destra, al massimo quattro cifre.
    timeDs_ = entering_ ? (timeDs_ % 1000) * 10 + arg : arg;
    entering_ = true;
  }
}

void Controller::update() {
  // Durante il Countdown il residuo arrotondato per eccesso: non mostra 0000 prima della fine.
  uint16_t ds = timing_.running() ? msToDs(timing_.remainingMs() + MS_PER_DS - 1) : timeDs_;
  if (ds != shownDs_) seg_.showDeciseconds(shownDs_ = ds);
}
