// Unità di tempo del dominio: millisecondi uint32_t all'interno, decimi di secondo
// come unità di interfaccia (REQ-11.5). Nessun header Arduino né AVR (REQ-11.1).
#pragma once

#include <stdint.h>

constexpr uint32_t MS_PER_DS = 100;
constexpr uint16_t DS_MIN = 1, DS_MAX = 9999;

constexpr uint32_t dsToMs(uint16_t ds) { return ds * MS_PER_DS; }
constexpr uint16_t msToDs(uint32_t ms) { return static_cast<uint16_t>(ms / MS_PER_DS); }
