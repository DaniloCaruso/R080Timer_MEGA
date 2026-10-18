#include "BuzzerTone.h"

#include <Arduino.h>
#include "Pins.h"

// La ISR di tone() scrive PL0 su PORTL, lo stesso registro del relè (PL2): per questo
// Timer1Clock scrive il relè dal main solo in ATOMIC_BLOCK.
void BuzzerTone::play(uint16_t hz, uint16_t ms) { tone(BUZZER_PIN, hz, ms); }
