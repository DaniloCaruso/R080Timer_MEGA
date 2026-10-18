#include "Controller.h"
#include "Precision.h"
#include "TimeUnits.h"
#include "TimingCore.h"

static uint16_t clampDs(int32_t ds) { return ds < DS_MIN ? DS_MIN : ds > DS_MAX ? DS_MAX : ds; }

void Controller::onInput(Input in, int8_t arg) {
  bool running = timing_.running();
  ChannelRecord& c = set_.active();
  if (in == Input::MainPress) {
    if (running) {
      timing_.cancel();
      beep(Sound::End);
    } else if (c.timeDs > 0) {
      // shortcut: ogni Modalità esegue un Countdown, finché Focus e Test strip non hanno il loro ticket.
      timing_.startCountdown(dsToMs(c.timeDs), c.relayEnabled);  // REQ-5.6: lampada spenta se disabilitata
      beep(Sound::Start);
      set_.save();  // REQ-6.7
    } else {
      beep(Sound::Error);  // REQ-3.8
    }
    entering_ = false;
    return;
  }
  if (running) return;  // REQ-5.9: durante l'Esposizione conta solo il pulsante principale
  bool typing = false;
  switch (in) {
    case Input::Digit:
      // Inserimento a calcolatrice: la cifra entra da destra, al massimo quattro cifre.
      c.timeDs = entering_ ? (c.timeDs % 1000) * 10 + arg : arg;
      typing = true;
      c.baseDs = c.timeDs;  // REQ-5.3
      c.offset = 0;
      break;
    case Input::Star:
    case Input::Hash: {
      int8_t dir = in == Input::Hash ? 1 : -1;
      if (c.precisionKind) stepFStop(c, dir);
      else c.timeDs = clampDs(c.timeDs + dir * int32_t(kSecondsSteps[c.precisionIndex]) * 10);
      break;
    }
    case Input::PrecTurn:
      c.precisionIndex = (c.precisionIndex + PRECISION_COUNT + arg) % PRECISION_COUNT;
      if (c.precisionKind && c.baseDs > 0)  // REQ-4.5
        c.timeDs = clampDs(fstopTime(c.baseDs, c.offset, kFStopDenominators[c.precisionIndex]));
      break;
    case Input::StarHold:
      set_.d.g.mute = !set_.d.g.mute;
      sound_.play(Sound::Confirm);  // REQ-5.5: la conferma suona anche entrando nel mute
      break;
    case Input::HashHold:
      c.relayEnabled = !c.relayEnabled;
      beep(Sound::Confirm);  // REQ-5.6
      break;
    case Input::PrecClick:
      c.precisionKind = !c.precisionKind;
      c.baseDs = c.timeDs;  // REQ-4.6
      c.offset = 0;
      break;
    case Input::Channel:
      set_.d.g.activeChannel = arg;  // REQ-6.2
      break;
    case Input::KeyD:
      c.timeDs = c.baseDs = 0;  // REQ-5.4
      c.offset = 0;
      break;
    case Input::ModeTurn:
      c.mode = (c.mode + MODE_COUNT + arg) % MODE_COUNT;  // REQ-5.10, REQ-6.3: il tempo resta
      break;
    default:
      break;
  }
  entering_ = typing;
  set_.changed();
}

// REQ-4.3: l'Offset si ferma a due cifre e dove il tempo uscirebbe da DS_MIN..DS_MAX. Il
// controllo guarda solo il verso del passo: dopo un cambio di precisione (REQ-4.5) l'Offset
// può essere oltre il limite, e il passo opposto deve poterlo riportare indietro.
void Controller::stepFStop(ChannelRecord& c, int8_t dir) {
  int8_t off = c.offset + dir;
  if (c.baseDs == 0 || off > 99 || off < -99) return;
  uint16_t t = fstopTime(c.baseDs, off, kFStopDenominators[c.precisionIndex]);
  if (dir > 0 ? t > DS_MAX : t < DS_MIN) return;
  c.offset = off;
  c.timeDs = clampDs(t);
}

void Controller::update() {
  uint8_t events = timing_.takeEvents();
  if (events & EV_END) beep(Sound::End);
  else if (events & EV_METRONOME) beep(Sound::Metronome);
  // Durante il Countdown il residuo arrotondato per eccesso: non mostra 0000 prima della fine.
  uint16_t ds = timing_.running() ? msToDs(timing_.remainingMs() + MS_PER_DS - 1) : set_.active().timeDs;
  if (ds != shownDs_) seg_.showDeciseconds(shownDs_ = ds);
}

void Controller::beep(Sound s) {
  if (!muted() || s == Sound::End) sound_.play(s);  // REQ-8.5
}
