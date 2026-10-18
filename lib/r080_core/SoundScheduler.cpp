#include "SoundScheduler.h"

struct Note {
  uint16_t hz, ms;  // hz = 0 è una pausa
};
struct Sequence {
  const Note* notes;
  uint8_t n;
};

// REQ-8.7: parametri dei suoni, ritoccabili durante il collaudo.
static constexpr Note kEnd[] = {{600, 150}, {0, 80}, {450, 150}, {0, 80}, {300, 150}};
static constexpr Note kError[] = {{150, 300}};
static constexpr Note kFocusWarn[] = {{450, 200}, {0, 100}, {450, 200}};
static constexpr Note kStart[] = {{1000, 60}};
static constexpr Note kStep[] = {{600, 150}};
static constexpr Note kPreWarn[] = {{800, 30}};  // un tick per evento: a -300, -200 e -100 ms dallo Step
static constexpr Note kMetronome[] = {{300, 80}};
static constexpr Note kConfirm[] = {{1200, 40}};

// Stesso ordine di enum Sound.
static constexpr Sequence kSounds[] = {
  {kEnd, 5},
  {kError, 1},
  {kFocusWarn, 3},
  {kStart, 1},
  {kStep, 1},
  {kPreWarn, 1},
  {kMetronome, 1},
  {kConfirm, 1},
};
static_assert(sizeof kSounds / sizeof kSounds[0] == static_cast<uint8_t>(Sound::Confirm) + 1,
              "kSounds e Sound non allineati");

void SoundScheduler::play(Sound s) {
  if (step_ != IDLE && s > cur_) return;
  cur_ = s;
  step_ = 0;
  startNow_ = true;
}

void SoundScheduler::tick(uint32_t nowMs) {
  if (step_ == IDLE) return;
  if (!startNow_ && (int32_t)(nowMs - nextMs_) < 0) return;
  const Sequence& q = kSounds[static_cast<uint8_t>(cur_)];
  if (step_ == q.n) {
    step_ = IDLE;  // finita l'ultima nota
    return;
  }
  const Note& note = q.notes[step_++];
  if (note.hz) buzzer_.play(note.hz, note.ms);
  nextMs_ = (startNow_ ? nowMs : nextMs_) + note.ms;
  startNow_ = false;
}
