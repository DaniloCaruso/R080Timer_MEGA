#include "SystemGuard.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/atomic.h>
#include "Pins.h"

constexpr uint32_t WDT_MARK = 0x57445452;  // 'WDTR': dopo un'accensione la RAM lo contiene per caso con probabilità 2^-32
static volatile uint32_t marker __attribute__((section(".noinit")));

// Senza bootloader, dopo un reset da watchdog questo resta acceso a 16 ms: spegnerlo prima di init() (avr-libc, wdt.h).
void wdtOffAtBoot() __attribute__((naked, used, section(".init3")));
void wdtOffAtBoot() {
  MCUSR = 0;
  wdt_disable();
}

// Primo timeout (250 ms): lampada spenta e marcatore. Poi si resta qui fino al secondo timeout
// (500 ms), che resetta la scheda: un marcatore scritto è sempre seguito dal reset (REQ-2.2).
ISR(WDT_vect) {
  RELAY_OFF();
  marker = WDT_MARK;
  for (;;) {}
}

bool SystemGuard::begin() {
  bool fromWatchdog = marker == WDT_MARK;
  marker = 0;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    wdt_reset();
    WDTCSR = _BV(WDCE) | _BV(WDE);  // sequenza temporizzata: la scrittura seguente entro 4 cicli
    WDTCSR = _BV(WDIE) | _BV(WDE) | _BV(WDP2);  // interrupt + reset, 32K cicli a 128 kHz = 250 ms
  }
  return fromWatchdog;
}
