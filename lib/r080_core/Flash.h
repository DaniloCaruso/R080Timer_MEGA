// Testi costanti in flash (CLAUDE.md, RAM): su AVR PROGMEM e le funzioni _P di avr-libc, sul PC
// costanti normali. Unico file del dominio che include un header AVR, e solo con __AVR__ (la CI lo esclude).
#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#define FSTR(s) PSTR(s)  // letterale in flash, solo dentro una funzione
#define fbyte(p) pgm_read_byte(p)
#define fmemcpy memcpy_P
#define fsnprintf snprintf_P  // formato in flash; gli argomenti %s restano in RAM
#define fvsnprintf vsnprintf_P
#else
#define PROGMEM
#define FSTR(s) (s)
#define fbyte(p) (*(p))
#define fmemcpy memcpy
#define fsnprintf snprintf
#define fvsnprintf vsnprintf
#endif
