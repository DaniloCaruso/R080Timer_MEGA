// Watchdog in modalità interrupt + reset, timeout 250 ms (TECHNICAL C-22). Il bootloader
// azzera MCUSR: la causa del reset la dice solo un marcatore in RAM non inizializzata.
// Il brown-out resta alla soglia di fabbrica dei fuse, 2,7 V (REQ-2.3): niente da fare qui.
#pragma once

namespace SystemGuard {
// Dopo Timer1Clock::begin(): avvia il watchdog e dice se l'avvio viene da un suo reset.
// Da qui loop() deve chiamare wdt_reset() almeno ogni 250 ms.
bool begin();
}
