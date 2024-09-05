// PRESENTATION_H
#ifndef PRESENTATION_H
#define PRESENTATION_H

#include <Arduino.h>
#include <ArduinoSTL.h>
#include "constants.h"
#include "variables.h"
#include "sound.h"
#include "functions/config.h"

// PROTOTYPES
void writePosition(int col, int row, String string);
void writeRow(int row, String string);
void showSound();
void showRel();
void showChannel();
void showPushOrPullActMode();
void showWelcomeMessage();
void clearPrecision();
void clearModality();
void showModality();
void showFocus();
void showCountdown();
void showTestStrip();

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
  writePosition(13,1,1);
  emitSound(TONE_DOWN, 200);
}

void showMute() {
  writePosition(13,1," ");
  emitSound(TONE_DOWN, 200);
}

void showTime(int ar_time) {
  sevenSegmentsDisplay.write(0x77);
  sevenSegmentsDisplay.write(0b00000100); // punto dec alla 1 cifra
  sprintf(keyBuffer, "%04i", ar_time);
  sevenSegmentsDisplay.write(keyBuffer);
}

void showRel() {
  lcd.setCursor(14,1);
  channelsRelayActivated[channel] ? lcd.write(2) : lcd.print(" ");
  emitSound(TONE_DOWN, 200);
}

void showChannel() {
  writePosition(8,1, CHANNELS_STR[channel-1]);
  showRel();
}

void showPushOrPullActMode() {
  int symbol = pushOrPullActMode == LOW ? 3 : 4;
  writePosition(15,1,symbol);
  emitSound(TONE_DOWN, 200);
}

void showWelcomeMessage() {
  writeRow(0, " - R080 Timer - " );
  writeRow(1, "    - MEGA -    " );
}

void clearPrecision() {  
  writeRow(1, "        ");
}

void clearModality() {
  writePosition(14,0,  "  ");
}

void showFocus() {
  writeRow(0, "FOCUS           ");
  clearPrecision();
  //beep(tone_down, 200); //beep al passaggio di programma
  delay(500);
  reset7SegmentsDisplay();
}

void showCountdown() {
  writeRow(0, "COUNTDOWN       ");
  //beep(tone_down, 200); //beep al passaggio di programma
  showTime(countdownTime[channel]);
}

void showTestStrip() {
  writeRow(0, "TEST STRIP      ");
  //beep(tone_down, 200); //beep al passaggio di programma
  showTime(testStripTime[channel]);
  showModality();
}

void showModality() {
  writePosition(14,0, operationStatus.current.modalityType.value);
}

#endif // PRESENTATION_H