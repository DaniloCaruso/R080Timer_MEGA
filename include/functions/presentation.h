// PRESENTATION_H
#ifndef PRESENTATION_H
#define PRESENTATION_H

#include <Arduino.h>
#include <ArduinoSTL.h>
#include "variables.h"
#include "functions/sound.h"

// PROTOTYPES
void writePosition(int col, int row, String string);
void writeRow(int row, String string);
void showSound();
void showRel();
void showChannel();
void showWelcomeMessage();
void clearPrecision();
void clearModality();
void showModalityType();
void showFocus();
void showCountdown();
void showTestStrip();
void showPrecisionType();
void showPrecision();

// IMPLEMENTATIONS
void writePosition(int col, int row, String string) {
  lcd.setCursor(col, row);
  lcd.print(string);
}

void writePosition(int col, int row, int symbol) {
  lcd.setCursor(col, row);
  lcd.write(symbol);
}

void writeRow(int row, String string) {
  writePosition(0, row, string);
}

void showSound() {
  writePosition(14,1,1);
  emitSound(TONE_DOWN, 200);
}

void showMute() {
  writePosition(14,1," ");
  emitSound(TONE_DOWN, 200);
}

void showTime(int ar_time) {
  sevenSegmentsDisplay.write(0x77);
  sevenSegmentsDisplay.write(0b00000100); // punto dec alla 1 cifra
  sprintf(keyBuffer, "%04i", ar_time);
  sevenSegmentsDisplay.write(keyBuffer);
}

void showRel() {
  lcd.setCursor(15,1);
  channelsRelayActivated[channel] ? lcd.write(2) : lcd.print(" ");
  emitSound(TONE_DOWN, 200);
}

void showChannel() {
  writePosition(9,1, CHANNELS_STR[channel-1]);
  showRel();
}

void showWelcomeMessage() {
  writeRow(0, " - R080 Timer - " );
  writeRow(1, "    - MEGA -    " );

  //Serial.println("[SETUP 5] WELCOME DISPLAYED");

}

void clearPrecision() {  
  writeRow(1, "        ");
  writePosition(11,0, operationStatus.current.precisionTypeStatus.current.value);
}

void clearModality() {
  writePosition(14,0,  "  ");
}

void showFocus() {
  writeRow(0, "FOCUS          ");
  clearPrecision();
  clearModality();
  //beep(tone_down, 200); //beep al passaggio di programma
  delay(500);
  reset7SegmentsDisplay();
}

void showCountdown() {
  writePosition(0,0, "COUNTDOWN [ ]   ");
  //beep(tone_down, 200); //beep al passaggio di programma
  showTime(countdownTime[channel]);
  if (operationStatus.current.isPrecisionTypeChanged()) {
    showPrecisionType();
  }
  if (operationStatus.current.isPrecisionChanged()) {
    showPrecision();
  }
}

void showTestStrip() {
  writePosition(0,0, "TEST STRIP[ ]   ");
  //beep(tone_down, 200); //beep al passaggio di programma
  showTime(testStripTime[channel]);
  if (operationStatus.current.isModalityTypeChanged()) {
    showModalityType();
  }
  if (operationStatus.current.isPrecisionTypeChanged()) {
    showPrecisionType();
  }
  if (operationStatus.current.isPrecisionChanged()) {
    showPrecision();
  }
}

void showModalityType() {
  writePosition(14,0, operationStatus.current.modalityTypeStatus.current.value);
}

void showPrecisionType() {
  writePosition(11,0, operationStatus.current.precisionTypeStatus.current.value);
}

void showPrecision() {
   writePosition(0,1, operationStatus.current.precisionStatus.current.string);
}

void clear7Seg() {
  sevenSegmentsDisplay.write(0x76);
  String message = "CLEAR";
  int messageLength = sizeof(message) - 1;
  int displayLength = 4;

  for (int i = 0; i < messageLength + displayLength; i++) {
    for (int j = 0; j < displayLength; j++) {
      int charIndex = i - (displayLength - 1) + j;
      (charIndex >= 0 && charIndex < messageLength) ? sevenSegmentsDisplay.write(message[charIndex]) : sevenSegmentsDisplay.write(' ');
    }

    delay(100);
    sevenSegmentsDisplay.write(0x76);  
  }
}


#endif // PRESENTATION_H