// Menu di servizio (TECHNICAL C-15): si apre tenendo `D` all'accensione. Encoder della
// Modalità per scorrere, suo pulsante per entrare o confermare, `D` per tornare indietro,
// cifre per i valori (REQ-9.14).
#pragma once

#include <stdint.h>
#include "Counters.h"
#include "Settings.h"
#include "SoundScheduler.h"

// Salute del sistema (REQ-9.4): la scrivono i driver e il loop di main.cpp, il menu la mostra.
struct Health {
  uint32_t loopMaxUs = 0, isrMaxUs = 0, lostTicks = 0;
};

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
  void compose(char rows[2][16]) const;  // le due righe dell'LCD

 private:
  void refuse();

  Settings& set_;
  Counters& cnt_;
  const Health& health_;
  SoundScheduler& sound_;
  const char* version_;
  const char* date_;
  bool open_ = false;
  bool inside_ = false;  // dentro una voce
  bool typed_ = false;   // falso: la prossima cifra inizia un valore nuovo
  uint8_t item_ = 0, page_ = 0, value_ = 0;
};
