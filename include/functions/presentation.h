// PRESENTATION_H
#ifndef PRESENTATION_H
#define PRESENTATION_H

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "variables.h"
#include "functions/sound.h"

// PROTOTYPES
void writePosition(int col, int row, const char* str);
void writePosition(int col, int row, int symbol);
void writeRow(int row, const char* str);
void showSound();
void showMute();
void showTime(int ar_time);
void showRel();
void showChannel();
void showWelcomeMessage();
void showModalityType();
void showModeName(uint8_t modeId);
void showFocus();
void showCountdown();
void showTestStrip();
void showPrecisionType();
void showPrecision();
void showFstopOffset();
void updateLCDRow1();

// IMPLEMENTATIONS
void writePosition(int col, int row, const char* str) {
  lcd.setCursor(col, row);
  while (*str) {
    lcd.write_char((uint8_t)*str++);
  }
}

void writePosition(int col, int row, int symbol) {
  lcd.setCursor(col, row);
  lcd.write_char(symbol);
}

void writeRow(int row, const char* str) {
  writePosition(0, row, str);
}

void showSound() {
  if (!mute) {
    writePosition(14,1,1); // icona nota
  } else {
    writePosition(14,1," ");
  }
}

void showMute() {
  writePosition(14,1," ");
}

void showTime(int ar_time) {
  sevenSegmentsDisplay.write(0x77);
  sevenSegmentsDisplay.write(0b00000100); // punto dec alla 1 cifra
  sprintf(keyBuffer, "%04i", ar_time);
  sevenSegmentsDisplay.write(keyBuffer);
}

void showRel() {
  lcd.setCursor(15,1);
  channelsRelayActivated[channel - 1] ? lcd.write_char(2) : lcd.write_char(' ');
}

void showChannel() {
  writePosition(9,1, CHANNELS_STR[channel-1]);
  showRel();
}

// Legge il nome modalita' dalla tabella PROGMEM in keyBuffer e lo scrive a (0,0).
void showModeName(uint8_t modeId) {
  const char* p = (const char*)pgm_read_ptr(&MODES_TEXTS[modeId]);
  strcpy_P(keyBuffer, p);
  writePosition(0, 0, keyBuffer);
}

// Mostra offset F-stop sulla riga 1 (pos 5-7)
void showFstopOffset() {
  if (operationStatus.current.precisionTypeStatus.current.id == FSTOP) {
    char buf[4];
    fstopState.formatOffset(buf);
    writePosition(5, 1, buf);
  } else {
    writePosition(5, 1, "   ");
  }
}

void showWelcomeMessage() {
  writeRow(0, " - R080 Timer - " );
  writeRow(1, "    - MEGA -    " );
}

// Aggiorna riga 1 LCD completa
//  Pos: 0123456789ABCDEF
//  R1:  Pr:1/3+02 Ch:A NOTA SPINA   (F-stop con offset)
//  R1:  Pr: 5s    Ch:A NOTA SPINA   (Seconds senza offset)
//  R1:           Ch:A NOTA SPINA   (Focus senza precisione)
void updateLCDRow1() {
  PrecisionType pt = operationStatus.current.precisionTypeStatus.current.id;
  if (pt != NO_PRECISION) {
    showPrecision();
    showFstopOffset();
  } else {
    writeRow(1, "         ");
  }
  showChannel();
  showSound();
  showRel();
}

void showFocus() {
  showModeName(FOCUS);
  writePosition(5, 0, "           ");  // pad col 5..15 (11 spazi)
  updateLCDRow1();
  reset7SegmentsDisplay();
}

void showCountdown() {
  showModeName(COUNTDOWN);
  writePosition(9, 0, "  [ ]  ");     // pad col 9..15 (7 char)
  showPrecisionType();
  updateLCDRow1();
  showTime(timeSet);
}

void showTestStrip() {
  showModeName(TEST_STRIP);
  writePosition(10, 0, " [ ]  ");     // pad col 10..15 (6 char)
  showPrecisionType();
  showModalityType();
  updateLCDRow1();
  showTime(timeSet);
}

void showModalityType() {
  writePosition(14,0, operationStatus.current.modalityTypeStatus.current.value);
}

void showPrecisionType() {
  writePosition(12,0, operationStatus.current.precisionTypeStatus.current.value);
}

void showPrecision() {
  // La stringa precisione (8 char + '\0') risiede in PROGMEM: copia in
  // keyBuffer prima di emetterla sul bus LCD char-by-char.
  strcpy_P(keyBuffer, operationStatus.current.precisionStatus.current.str);
  writePosition(0, 1, keyBuffer);
}

#endif // PRESENTATION_H
