// CONFIG_H
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoSTL.h>
#include "variables.h"
#include "constants.h"

// PROTOTYPES
void initializePinMode();
void configKeypad();
void configLCD();
void config7SegmentsDisplay();
void reset7SegmentsDisplay();
void resetLCD();

// IMPLEMENTATIONS
void initializePinMode() {

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(MAIN_BTN_PIN, INPUT);

  // Serial.println("[SETUP 1] PINMODE INITIALIZED");
}

void resetTime() {
  reset7SegmentsDisplay();
  timeSet = 0;
  nCifra = 0;
  appoTimeSet = 0;
}

void configKeypad() {
  
  keypad.setDebounceTime(50);
  // Serial.println("[SETUP 2] KEYPAD CONFIGURED");
}

//TODO: DA MODIFICARE CON WONDERSHARE
void configLCD() {

  lcd.init(); // inizializza display lcd
  lcd.backlight(); // attiva la retroilluminazione
  lcd.createChar(1, note); //Inizializzo caratteri speciali Nota
  lcd.createChar(2, spina); //Inizializzo caratteri speciali Orologio
  lcd.createChar(3, frecciasu); //Inizializzo caratteri speciali Freccia in Su
  lcd.createChar(4, frecciagiu); //Inizializzo caratteri speciali Freccia in Giù

  // Serial.println("[SETUP 3] LCD CONFIGURED");
}

void config7SegmentsDisplay() {

  sevenSegmentsDisplay.begin(9600); // connette il display 7 segmenti display in modo seriale
  sevenSegmentsDisplay.write(0x76); // Display reset
  sevenSegmentsDisplay.write(0x7A); // Comando per controllo luminosità
  sevenSegmentsDisplay.write((byte) brightness);

  // Serial.println("[SETUP 4] 7SEG CONFIGURED");
}

void reset7SegmentsDisplay() {
    sevenSegmentsDisplay.write(0x76);
    sevenSegmentsDisplay.print("0000");
    sevenSegmentsDisplay.write(0x77);
    sevenSegmentsDisplay.write(0b00000100); // punto dec alla 1 cifra

    // Serial.println("[SETUP 6] 7SEG RESETTED");

}

void resetLCD() {
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print("                ");

  // Serial.println("[SETUP 7] LCD RESETTED");

}


#endif // CONFIG_H