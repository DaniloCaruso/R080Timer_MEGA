#include "Controller.h"

#include <stdio.h>
#include "Precision.h"
#include "TimeUnits.h"
#include "TimingCore.h"

// Un passo di luminosità, fermo ai livelli 1..LEVEL_COUNT.
static void stepLevel(uint8_t& level, int8_t dir) {
  if (dir > 0 ? level < LEVEL_COUNT : level > 1) level += dir;
}

static uint16_t clampDs(int32_t ds) { return ds < DS_MIN ? DS_MIN : ds > DS_MAX ? DS_MAX : ds; }

void Controller::boot(bool watchdogReset, bool settingsOk, bool keyD) {
  log_.line(watchdogReset ? "BOOT reset=WDT" : "BOOT reset=POWER");  // REQ-9.10
  if (!settingsOk) log_.line("ERROR settings=factory");
  if (keyD) menu_.open();
}

void Controller::onInput(Input in, int8_t arg) {
  if (menu_.active()) {  // REQ-9.14: nel Menu di servizio nessuna Esposizione
    if (menu_.testing() && in != Input::KeyD) menu_.test(in, arg);
    else if (in == Input::ModeTurn) menu_.turn(arg);
    else if (in == Input::ModeClick) menu_.click();
    else if (in == Input::KeyD) menu_.back();
    else if (in == Input::Digit) menu_.digit(arg);
    return;
  }
  if (in == Input::MainRelease) return;  // serve solo al test del pulsante (REQ-9.5)
  update();  // prima gli eventi della ISR: uno Step appena scattato mette già in pausa
  bool running = timing_.running();
  ChannelRecord& c = set_.active();
  if (in == Input::MainPress) {
    if (running) {
      timing_.cancel();
      beep(Sound::End);
      exposureEnd("CANCEL");
    } else if (paused_) {
      // REQ-3.12: solo l'incremento fino allo Step successivo, già calcolato allo Step.
      paused_ = false;
      timing_.startStrip(true, timing_.elapsedMs(), stripDs_ * MS_PER_DS, c.relayEnabled);
      beep(Sound::Start);
    } else if (c.mode == FOCUS || c.timeDs > 0) {
      // REQ-5.6: lampada spenta se disabilitata
      uint32_t ms = dsToMs(c.timeDs);
      if (c.mode == FOCUS) {  // REQ-3.2, REQ-3.3
        ms = kFocusLimitMinutes[set_.d.g.focusLimitIndex] * 60000UL;
        timing_.startFocus(ms, c.relayEnabled);
      } else if (c.mode == TEST_STRIP) {
        stripK_ = 0;  // REQ-3.13: ogni prova riparte dal primo Step
        stripDs_ = c.timeDs;
        timing_.startStrip(c.advance, 0, ms, c.relayEnabled);
      } else {
        timing_.startCountdown(ms, c.relayEnabled);
      }
      logStart(c, ms);
      beep(Sound::Start);
      set_.save();  // REQ-6.7
    } else {
      beep(Sound::Error);  // REQ-3.8
    }
    entering_ = false;
    return;
  }
  if (running) return;  // REQ-5.9: durante l'Esposizione conta solo il pulsante principale
  if (paused_) {
    // REQ-3.16: D, A/B/C e il cambio di Modalità concludono la prova; il resto è ignorato.
    if (in == Input::KeyD || in == Input::Channel || in == Input::ModeTurn) {
      paused_ = false;
      timing_.cancel();
      beep(Sound::End);
      exposureEnd("END");  // la prova passo-passo è conclusa: una sola Esposizione
    }
    return;
  }
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
  if (menu_.active()) timing_.setRelay(menu_.relayOn());  // REQ-9.5: spento appena si lascia il test
  uint8_t events = timing_.takeEvents();
  if (events & (EV_END | EV_FOCUS_OFF)) exposureEnd("END");
  if ((events & EV_STEP) && !(events & EV_END)) {
    nextStripStep();
    paused_ = set_.active().advance;  // REQ-3.12: il TimingCore ha già spento il relè
  }
  if (events & (EV_END | EV_FOCUS_OFF)) beep(Sound::End);
  else if (events & EV_FOCUS_WARN) beep(Sound::FocusWarn);
  else if (events & EV_STEP) beep(Sound::Step);
  else if (events & EV_PREWARN) beep(Sound::PreWarn);
  else if (events & EV_METRONOME) beep(Sound::Metronome);
  bool running = timing_.running();
  uint8_t mode = set_.active().mode;
  stripShown_ = paused_ || (running && mode == TEST_STRIP) ? stripK_ + 1 : 0;
  uint16_t ds = set_.active().timeDs;
  if (paused_) {
    // REQ-7.8: tempo cumulativo dello Step successivo, al massimo 999,9 s perché lì la prova si conclude.
    ds = stripDs_ > DS_MAX ? DS_MAX : stripDs_;
  } else if (running && mode != COUNTDOWN) {
    // Test strip: tempo cumulativo (REQ-7.7). Focus: tempo trascorso (REQ-3.2).
    ds = msToDs(timing_.elapsedMs());
    // shortcut: il 7 segmenti ha 4 cifre, con il Focus di 20 min resta fermo a 999,9 s; da rivedere se serve.
    if (ds > DS_MAX) ds = DS_MAX;
  } else if (running) {
    // Countdown: residuo arrotondato per eccesso, non mostra 0000 prima della fine.
    ds = msToDs(timing_.remainingMs() + MS_PER_DS - 1);
  }
  if (menu_.sevenSegTest()) ds = 8888;  // REQ-9.5: tutti i segmenti delle cifre e il punto decimale
  if (ds != shownDs_) seg_.showDeciseconds(shownDs_ = ds);
  if (set_.d.g.segLevel != segLevel_) seg_.setBrightness(segLevel_ = set_.d.g.segLevel);  // REQ-7.4
}

// REQ-9.10: inizio di ogni Esposizione nel log, come `START mode=CD ch=A ms=10000`.
void Controller::logStart(const ChannelRecord& c, uint32_t ms) {
  static const char kModes[MODE_COUNT][3] = {"FO", "CD", "TS"};
  char b[40];
  snprintf(b, sizeof b, "START mode=%s ch=%c ms=%lu", kModes[c.mode], 'A' + set_.d.g.activeChannel, (unsigned long)ms);
  log_.line(b);
  startMs_ = ms;
}

// REQ-9.10, REQ-9.11: fine o Annullamento nel log con la durata misurata dalla ISR, e contatori.
// Il Focus non è un'Esposizione ma il suo tempo a relè acceso va nelle ore di lampada.
void Controller::exposureEnd(const char* how) {
  const ChannelRecord& c = set_.active();
  uint32_t measured = timing_.elapsedMs();
  cnt_.exposureEnd(c.mode != FOCUS, c.relayEnabled ? measured : 0);
  char b[48];
  snprintf(b, sizeof b, "%s ms=%lu measured=%lu", how, (unsigned long)startMs_, (unsigned long)measured);
  log_.line(b);
}

void Controller::beep(Sound s) {
  if (!muted() || s == Sound::End) sound_.play(s);  // REQ-8.5
}
