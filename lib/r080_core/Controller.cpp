#include "Controller.h"
#include "Precision.h"
#include "TimeUnits.h"

static uint16_t clampDs(int32_t ds) { return ds < DS_MIN ? DS_MIN : ds > DS_MAX ? DS_MAX : ds; }

void Controller::onInput(Input in, int8_t arg) {
  bool running = timing_.running();
  if (in == Input::MainPress) {
    if (running) timing_.cancel();
    else if (timeDs_ > 0) timing_.startCountdown(dsToMs(timeDs_));
    entering_ = false;
    return;
  }
  if (running) return;  // REQ-5.9: durante l'Esposizione conta solo il pulsante principale
  switch (in) {
    case Input::Digit:
      // Inserimento a calcolatrice: la cifra entra da destra, al massimo quattro cifre.
      timeDs_ = entering_ ? (timeDs_ % 1000) * 10 + arg : arg;
      entering_ = true;
      baseDs_ = timeDs_;  // REQ-5.3
      offset_ = 0;
      return;
    case Input::Star:
    case Input::Hash: {
      int8_t dir = in == Input::Hash ? 1 : -1;
      if (fstop_) stepFStop(dir);
      else timeDs_ = clampDs(timeDs_ + dir * int32_t(kSecondsSteps[precIdx_]) * 10);
      break;
    }
    case Input::PrecTurn:
      precIdx_ = (precIdx_ + PRECISION_COUNT + arg) % PRECISION_COUNT;
      if (fstop_ && baseDs_ > 0) timeDs_ = clampDs(fstopTime(baseDs_, offset_, kFStopDenominators[precIdx_]));  // REQ-4.5
      break;
    case Input::PrecClick:
      fstop_ = !fstop_;
      baseDs_ = timeDs_;  // REQ-4.6
      offset_ = 0;
      break;
    default:
      break;
  }
  entering_ = false;
}

// REQ-4.3: l'Offset si ferma a due cifre e dove il tempo uscirebbe da DS_MIN..DS_MAX. Il
// controllo guarda solo il verso del passo: dopo un cambio di precisione (REQ-4.5) l'Offset
// può essere oltre il limite, e il passo opposto deve poterlo riportare indietro.
void Controller::stepFStop(int8_t dir) {
  int8_t off = offset_ + dir;
  if (baseDs_ == 0 || off > 99 || off < -99) return;
  uint16_t t = fstopTime(baseDs_, off, kFStopDenominators[precIdx_]);
  if (dir > 0 ? t > DS_MAX : t < DS_MIN) return;
  offset_ = off;
  timeDs_ = clampDs(t);
}

void Controller::update() {
  // Durante il Countdown il residuo arrotondato per eccesso: non mostra 0000 prima della fine.
  uint16_t ds = timing_.running() ? msToDs(timing_.remainingMs() + MS_PER_DS - 1) : timeDs_;
  if (ds != shownDs_) seg_.showDeciseconds(shownDs_ = ds);
}
