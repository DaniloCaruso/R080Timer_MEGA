#include "Controller.h"

#include "Precision.h"
#include "TimeUnits.h"
#include "TimingCore.h"

// Un passo di luminosità, fermo ai livelli 1..LEVEL_COUNT.
static void stepLevel(uint8_t& level, int8_t dir) {
  if (dir > 0 ? level < LEVEL_COUNT : level > 1) level += dir;
}

constexpr uint16_t MIN_SEC = 0x8000;  // in shownDs_: il valore è mmss, non decimi

// Tempo del Canale spostato di n passi di precisione, senza limiti (chi chiama confronta con
// DS_MIN..DS_MAX): F-Stop sempre da Tempo base e Offset, Secondi dal tempo impostato.
static int32_t shiftedDs(const ChannelRecord& c, int16_t n) {
  if (c.precisionKind) return fstopTime(c.baseDs, c.offset + n, kFStopDenominators[c.precisionIndex]);
  return c.timeDs + int32_t(n) * kSecondsSteps[c.precisionIndex] * 10;
}

static uint16_t clampDs(int32_t ds) { return ds < DS_MIN ? DS_MIN : ds > DS_MAX ? DS_MAX : ds; }

void Controller::boot(bool watchdogReset, bool settingsOk, bool keyD) {
  log_.print(watchdogReset ? FSTR("BOOT reset=WDT") : FSTR("BOOT reset=POWER"));  // REQ-9.10
  if (!settingsOk) log_.print(FSTR("ERROR settings=factory"));
  if (keyD) menu_.open();
}

void Controller::onInput(Input in, int8_t arg) {
  if (menu_.active()) {  // REQ-9.14: nel Menu di servizio nessuna Esposizione
    if (menu_.testing() && in != Input::KeyD) menu_.test(in, arg);
    else if (in == Input::ModeTurn) menu_.turn(arg);
    else if (in == Input::ModeClick) menu_.click();
    else if (in == Input::KeyD) menu_.back();
    else if (in == Input::Digit) menu_.digit(arg);
    else if (in == Input::Star) menu_.negate();
    return;
  }
  if (in == Input::MainRelease) return;  // serve solo al test del pulsante (REQ-9.5)
  update();  // prima gli eventi della ISR: uno Step appena scattato mette già in pausa
  bool running = timing_.running();
  // Un'Esposizione finita dopo la raccolta: da fermo la ISR non genera altri eventi, raccolti
  // ora tutti prima del comando (che li scarterebbe).
  if (!running) update();
  ChannelRecord& c = set_.active();
  if (in == Input::MainPress) {
    if (running) {
      timing_.cancel();
      beep(Sound::End);
      exposureEnd(true);
    } else if (paused_) {
      // REQ-3.12: solo l'incremento fino allo Step successivo, già calcolato allo Step.
      paused_ = false;
      timing_.startStrip(true, timing_.elapsedMs(), stripDs_ * MS_PER_DS, c.relayEnabled);
      beep(Sound::Start);
    } else if (c.mode == FOCUS || c.timeDs > 0) {
      // REQ-5.6: lampada spenta se disabilitata
      uint32_t ms = dsToMs(c.timeDs);
      if (c.mode == FOCUS) {  // REQ-3.2, REQ-3.3
        ms = kFocusLimitMinutes[g().focusLimitIndex] * 60000UL;
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
    if (in != Input::KeyD && in != Input::Channel && in != Input::ModeTurn) return;
    paused_ = false;
    timing_.cancel();
    beep(Sound::End);
    exposureEnd(false);  // la prova passo-passo è conclusa: una sola Esposizione
    if (in == Input::KeyD) return;  // D conclude soltanto; A/B/C e l'encoder fanno anche la loro azione
  }
  // REQ-4.6, REQ-5.1: in Focus niente precisione né tempo da tastierino.
  if (c.mode == FOCUS && (in == Input::Digit || in == Input::PrecTurn || in == Input::PrecClick)) return;
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
      if (c.mode == FOCUS) stepLevel(g().lcdLevel, dir);  // REQ-5.7
      else if (c.precisionKind) stepFStop(c, dir);
      else c.timeDs = clampDs(shiftedDs(c, dir));
      break;
    }
    case Input::ZeroStar:
    case Input::ZeroHash:
      if (c.mode == FOCUS) stepLevel(g().segLevel, in == Input::ZeroHash ? 1 : -1);  // REQ-5.8
      break;
    case Input::PrecTurn:
      c.precisionIndex = (c.precisionIndex + PRECISION_COUNT + arg) % PRECISION_COUNT;
      if (c.precisionKind && c.baseDs > 0)  // REQ-4.5
        c.timeDs = clampDs(shiftedDs(c, 0));
      break;
    case Input::StarHold:
      g().mute = !g().mute;
      beep(Sound::Confirm);  // REQ-5.5: la conferma suona solo uscendo dal mute
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
      g().activeChannel = arg;  // REQ-6.2
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
  int32_t t = shiftedDs(c, dir);
  if (dir > 0 ? t > DS_MAX : t < DS_MIN) return;
  c.offset = off;
  c.timeDs = clampDs(t);
}

// REQ-3.9, REQ-3.10: lo Step k (da 0) si calcola sempre dal Tempo base, mai dallo Step
// precedente, così l'arrotondamento non si accumula. Float: solo nel loop.
void Controller::nextStripStep() {
  uint32_t ds = shiftedDs(set_.active(), ++stripK_);
  // REQ-3.10: con 1/64 e base piccola l'arrotondamento darebbe Step uguali.
  stripDs_ = ds > stripDs_ ? ds : stripDs_ + 1;
  timing_.setNextStep(stripDs_ * MS_PER_DS);  // oltre 999,9 s conclude prima il TimingCore
}

void Controller::update() {
  if (menu_.active()) timing_.setRelay(menu_.relayOn());  // REQ-9.5: spento appena si lascia il test
  uint8_t events = timing_.takeEvents();
  if (events & (EV_END | EV_FOCUS_OFF)) exposureEnd(false);
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
    // Test strip: tempo cumulativo (REQ-7.7). Focus: tempo trascorso (REQ-3.2), oltre 999,9 s in
    // minuti.secondi (il test strip si conclude prima).
    ds = msToDs(timing_.elapsedMs());
    if (ds > DS_MAX) ds = MIN_SEC | (ds / 600 * 100 + ds / 10 % 60);
  } else if (running) {
    // Countdown: residuo arrotondato per eccesso, non mostra 0000 prima della fine.
    ds = msToDs(timing_.remainingMs() + MS_PER_DS - 1);
  }
  if (menu_.sevenSegTest()) ds = 8888;  // REQ-9.5: tutti i segmenti delle cifre e il punto decimale
  if (ds != shownDs_) {
    shownDs_ = ds;
    if (ds & MIN_SEC) seg_.showMinSec(ds & ~MIN_SEC);
    else seg_.showDeciseconds(ds);
  }
  if (g().segLevel != segLevel_) seg_.setBrightness(segLevel_ = g().segLevel);  // REQ-7.4
  if (g().clockPpm != ppm_) timing_.setCorrection(ppm_ = g().clockPpm);  // REQ-1.7
  calibrationLog();
}

// REQ-9.6: una riga al secondo con i tick di Timer1 contati al momento della riga, senza
// correzione: tools/calibrate.py li confronta con l'orologio del PC.
void Controller::calibrationLog() {
  if (!menu_.calibrating()) {
    calNextMs_ = 0;
    return;
  }
  uint32_t ms = timing_.tickCount();
  if (!calNextMs_) calNextMs_ = ms + 1000;
  if (ms < calNextMs_) return;
  calNextMs_ += 1000;
  log_.print(FSTR("CAL ms=%lu"), (unsigned long)ms);
}

// REQ-9.10: inizio di ogni Esposizione nel log, come `START mode=CD ch=A ms=10000`.
void Controller::logStart(const ChannelRecord& c, uint32_t ms) {
  static const char kModes[] PROGMEM = "FOCDTS";  // due lettere per Modalità
  log_.print(FSTR("START mode=%c%c ch=%c ms=%lu"), fbyte(&kModes[2 * c.mode]), fbyte(&kModes[2 * c.mode + 1]),
             'A' + g().activeChannel, (unsigned long)ms);
  startMs_ = ms;
}

// REQ-9.10, REQ-9.11: fine o Annullamento nel log con la durata misurata dalla ISR, e contatori.
// Il Focus non è un'Esposizione ma il suo tempo a relè acceso va nelle ore di lampada.
void Controller::exposureEnd(bool cancel) {
  const ChannelRecord& c = set_.active();
  uint32_t measured = timing_.elapsedMs();
  cnt_.exposureEnd(c.mode != FOCUS, c.relayEnabled ? measured : 0);
  log_.print(cancel ? FSTR("CANCEL ms=%lu measured=%lu") : FSTR("END ms=%lu measured=%lu"), (unsigned long)startMs_,
             (unsigned long)measured);
}

void Controller::beep(Sound s) {
  if (!muted() || s == Sound::End) sound_.play(s);  // REQ-8.5
}
