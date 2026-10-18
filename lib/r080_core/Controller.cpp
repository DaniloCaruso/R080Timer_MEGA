#include "Controller.h"
#include "Precision.h"
#include "TimeUnits.h"
#include "TimingCore.h"

// Un passo di luminosità, fermo ai livelli 1..LEVEL_COUNT.
static void stepLevel(uint8_t& level, int8_t dir) {
  if (dir > 0 ? level < LEVEL_COUNT : level > 1) level += dir;
}

static uint16_t clampDs(int32_t ds) { return ds < DS_MIN ? DS_MIN : ds > DS_MAX ? DS_MAX : ds; }

void Controller::onInput(Input in, int8_t arg) {
  bool running = timing_.running();
  ChannelRecord& c = set_.active();
  if (in == Input::MainPress) {
    if (running) {
      timing_.cancel();
      beep(Sound::End);
    } else if (c.mode == FOCUS || c.timeDs > 0) {
      // REQ-5.6: lampada spenta se disabilitata
      if (c.mode == FOCUS) {  // REQ-3.2, REQ-3.3
        timing_.startFocus(kFocusLimitMinutes[set_.d.g.focusLimitIndex] * 60000UL, c.relayEnabled);
      } else if (c.mode == TEST_STRIP) {
        stripK_ = 0;  // REQ-3.13: ogni prova riparte dal primo Step
        stripDs_ = c.timeDs;
        timing_.startStrip(dsToMs(c.timeDs), c.relayEnabled);
      } else {
        timing_.startCountdown(dsToMs(c.timeDs), c.relayEnabled);
      }
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
      if (c.mode == FOCUS) stepLevel(set_.d.g.lcdLevel, dir);  // REQ-5.7
      else if (c.precisionKind) stepFStop(c, dir);
      else c.timeDs = clampDs(c.timeDs + dir * int32_t(kSecondsSteps[c.precisionIndex]) * 10);
      break;
    }
    case Input::ZeroStar:
    case Input::ZeroHash:
      if (c.mode == FOCUS) stepLevel(set_.d.g.segLevel, in == Input::ZeroHash ? 1 : -1);  // REQ-5.8
      break;
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
    case Input::ModeClick:
      // shortcut: il passo-passo si sceglie ma il test strip gira ancora continuo; lo esegue la issue #10.
      if (c.mode == TEST_STRIP) c.advance = !c.advance;  // REQ-4.7
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

// REQ-3.9, REQ-3.10: lo Step k (da 0) si calcola sempre dal Tempo base, mai dallo Step
// precedente, così l'arrotondamento non si accumula. Float: solo nel loop.
uint32_t Controller::stripStepDs(uint16_t k) const {
  const ChannelRecord& c = set_.active();
  if (c.precisionKind) return fstopTime(c.baseDs, c.offset + k, kFStopDenominators[c.precisionIndex]);
  return c.timeDs + uint32_t(k) * kSecondsSteps[c.precisionIndex] * 10;
}

void Controller::nextStripStep() {
  uint32_t ds = stripStepDs(++stripK_);
  // REQ-3.10: con 1/64 e base piccola l'arrotondamento darebbe Step uguali.
  stripDs_ = ds > stripDs_ ? ds : stripDs_ + 1;
  timing_.setNextStep(stripDs_ * MS_PER_DS);  // oltre 999,9 s conclude prima il TimingCore
}

void Controller::update() {
  uint8_t events = timing_.takeEvents();
  if ((events & EV_STEP) && !(events & EV_END)) nextStripStep();
  if (events & (EV_END | EV_FOCUS_OFF)) beep(Sound::End);
  else if (events & EV_FOCUS_WARN) beep(Sound::FocusWarn);
  else if (events & EV_STEP) beep(Sound::Step);
  else if (events & EV_PREWARN) beep(Sound::PreWarn);
  else if (events & EV_METRONOME) beep(Sound::Metronome);
  bool running = timing_.running();
  uint8_t mode = set_.active().mode;
  stripShown_ = running && mode == TEST_STRIP ? stripK_ + 1 : 0;
  uint16_t ds = set_.active().timeDs;
  if (running && mode != COUNTDOWN) {
    // Test strip: tempo cumulativo (REQ-7.7). Focus: tempo trascorso (REQ-3.2).
    ds = msToDs(timing_.elapsedMs());
    // shortcut: il 7 segmenti ha 4 cifre, con il Focus di 20 min resta fermo a 999,9 s; da rivedere se serve.
    if (ds > DS_MAX) ds = DS_MAX;
  } else if (running) {
    // Countdown: residuo arrotondato per eccesso, non mostra 0000 prima della fine.
    ds = msToDs(timing_.remainingMs() + MS_PER_DS - 1);
  }
  if (ds != shownDs_) seg_.showDeciseconds(shownDs_ = ds);
  if (set_.d.g.segLevel != segLevel_) seg_.setBrightness(segLevel_ = set_.d.g.segLevel);  // REQ-7.4
}

void Controller::beep(Sound s) {
  if (!muted() || s == Sound::End) sound_.play(s);  // REQ-8.5
}
