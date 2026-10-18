#include "LcdWaveshare.h"

#include <Arduino.h>
#include <Wire.h>

static constexpr uint8_t LCD_ADDR = 0x3E, RGB_ADDR = 0x60;
static constexpr uint8_t REG_MODE1 = 0x00, REG_MODE2 = 0x01, REG_BLUE = 0x02, REG_GREEN = 0x03,
                         REG_RED = 0x04, REG_OUTPUT = 0x08;
static constexpr uint8_t CMD = 0x80, DATA = 0x40;  // byte di controllo: comando seguito da altro, dati
static constexpr uint8_t FUNCTION_SET = 0x28;      // 2 righe, 5x8
static constexpr uint8_t READY = 4;
static constexpr uint32_t RETRY_MS = 2000;
// REQ-7.11: livelli 1..8 del rosso, più fitti in basso.
static constexpr uint8_t kLcdLevels[LEVEL_COUNT] = {4, 8, 16, 28, 48, 80, 140, 255};

// Caratteri personalizzati 1 (nota) e 2 (spina), contigui in CGRAM dall'indirizzo 8.
static const uint8_t GLYPHS[16] = {
  0x02, 0x03, 0x02, 0x02, 0x0E, 0x1E, 0x0C, 0x00,
  0x0A, 0x0A, 0x1F, 0x11, 0x11, 0x0E, 0x04, 0x04,
};

static bool send(uint8_t addr, const uint8_t* data, uint8_t n) {
  Wire.beginTransmission(addr);
  Wire.write(data, n);
  return Wire.endTransmission() == 0;  // anche 5 = timeout
}

static bool command(uint8_t c) {
  const uint8_t d[] = {CMD, c};
  return send(LCD_ADDR, d, sizeof d);
}

static bool setReg(uint8_t reg, uint8_t v) {
  const uint8_t d[] = {reg, v};
  return send(RGB_ADDR, d, sizeof d);
}

void LcdWaveshare::begin() {
  Wire.begin();
  Wire.setClock(100000);
  Wire.setWireTimeout(3000, true);  // REQ-2.7: un LCD scollegato non blocca il bus
}

// Ogni passo invia pochi comandi e fissa l'attesa che il controller richiede prima del successivo.
bool LcdWaveshare::initStep() {
  uint32_t now = millis();
  switch (step_) {
    case 0:
    case 1:
      nextMs_ = now + 5;  // function set ripetuto, più di 4,1 ms tra i due
      return command(FUNCTION_SET);
    case 2:
      nextMs_ = now + 2;  // clear richiede 1,53 ms
      return command(FUNCTION_SET) && command(0x0C) && command(0x01);  // display acceso, clear
    default: {
      uint8_t cgram[3 + sizeof GLYPHS] = {CMD, 0x40 | 8, DATA};
      memcpy(cgram + 3, GLYPHS, sizeof GLYPHS);
      // REQ-7.1: sempre e solo rosso; MODE2 senza blink.
      return command(0x06) && send(LCD_ADDR, cgram, sizeof cgram) && setReg(REG_MODE1, 0) &&
             setReg(REG_OUTPUT, 0xFF) && setReg(REG_MODE2, 0x00) && setReg(REG_RED, kLcdLevels[level_ - 1]) &&
             setReg(REG_GREEN, 0) && setReg(REG_BLUE, 0);
    }
  }
}

void LcdWaveshare::fail() {
  online_ = false;
  step_ = 0;
  nextMs_ = millis() + RETRY_MS;
}

bool LcdWaveshare::online() {
  if (step_ == READY) return true;
  if ((int32_t)(millis() - nextMs_) < 0) return online_;
  if (!initStep()) fail();
  else if (++step_ == READY) online_ = true;
  return online_;
}

void LcdWaveshare::setBrightness(uint8_t level) {
  level_ = level;  // da fermo lo applica la prossima inizializzazione
  if (step_ == READY && !setReg(REG_RED, kLcdLevels[level - 1])) fail();
}

bool LcdWaveshare::writeAt(uint8_t col, uint8_t row, const char* s, uint8_t n) {
  if (step_ != READY) return false;
  // Una sola transazione: indirizzo DDRAM, poi i caratteri (al massimo 3 + 16 byte nel buffer Wire da 32).
  Wire.beginTransmission(LCD_ADDR);
  Wire.write(CMD);
  Wire.write((row ? 0xC0 : 0x80) | col);
  Wire.write(DATA);
  Wire.write(reinterpret_cast<const uint8_t*>(s), n);
  if (Wire.endTransmission() == 0) return true;
  fail();
  return false;
}
