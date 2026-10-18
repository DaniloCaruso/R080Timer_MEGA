// Menu di servizio (TECHNICAL C-15): si apre tenendo `D` all'accensione. Encoder della
// Modalità per scorrere, suo pulsante per entrare o confermare, `D` per tornare indietro,
// cifre per i valori (REQ-9.14).
#pragma once

#include <stdint.h>
#include "Counters.h"
#include "Hal.h"
#include "Settings.h"
#include "SoundScheduler.h"

class ServiceMenu {
 public:
  ServiceMenu(Settings& settings, Counters& counters, const Health& health, SoundScheduler& sound,
              const char* version, const char* buildDate)
      : set_(settings), cnt_(counters), health_(health), sound_(sound), version_(version), date_(buildDate) {}
  void open() { open_ = true; }
  bool active() const { return open_; }
  void turn(int8_t dir);
  void click();
  void back();
  void digit(uint8_t d);
  void negate();  // `*`: cambia il segno del valore (Correzione del clock)
  bool calibrating() const { return log_; }  // log di calibrazione attivo (REQ-9.6)
  void compose(char rows[2][16]) const;  // le due righe dell'LCD
  // Test hardware (REQ-9.5): dentro un test ogni input va a test(), tranne `D` che esce.
  bool testing() const;
  void test(Input in, int8_t arg);
  bool relayOn() const;  // il Controller lo applica al relè finché il menu è aperto
  bool sevenSegTest() const;  // il Controller mostra 888.8 sul 7 segmenti

 private:
  void refuse();
  void load();  // valore iniziale della voce e della schermata

  Settings& set_;
  Counters& cnt_;
  const Health& health_;
  SoundScheduler& sound_;
  const char* version_;
  const char* date_;
  bool open_ = false;
  bool inside_ = false;  // dentro una voce
  bool typed_ = false;   // falso: la prossima cifra inizia un valore nuovo
  bool log_ = false;
  uint8_t item_ = 0, page_ = 0;
  int32_t value_ = 0;
  int8_t hw_[4] = {};  // stato del test hardware in corso, azzerato entrando
};
