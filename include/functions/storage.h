#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <avr/eeprom.h>
#include "variables.h"

// EEPROM Layout
// Addr 0:    Magic byte (0xA5 indica dati validi)
// Addr 1:    brightness (byte, 1-10)
// Addr 2:    mute (byte, 0 o 1)
// Addr 3:    canale attivo (byte, 1-3)
// Addr 4-5:  tempo canale A (int16_t, decisecondi)
// Addr 6-7:  tempo canale B (int16_t)
// Addr 8-9:  tempo canale C (int16_t)
// Addr 10:   relay canale A abilitato (byte)
// Addr 11:   relay canale B abilitato (byte)
// Addr 12:   relay canale C abilitato (byte)

const uint8_t EEPROM_MAGIC = 0xA5;

// Indirizzi EEPROM
const int EE_MAGIC       = 0;
const int EE_BRIGHTNESS  = 1;
const int EE_MUTE        = 2;
const int EE_CHANNEL     = 3;
const int EE_CH_A_TIME   = 4;   // 2 bytes
const int EE_CH_B_TIME   = 6;   // 2 bytes
const int EE_CH_C_TIME   = 8;   // 2 bytes
const int EE_CH_A_RELAY  = 10;
const int EE_CH_B_RELAY  = 11;
const int EE_CH_C_RELAY  = 12;

// PROTOTYPES
bool loadSettings();
void saveSettings();

// IMPLEMENTATIONS

bool loadSettings() {
  if (eeprom_read_byte((uint8_t*)EE_MAGIC) != EEPROM_MAGIC) {
    return false; // Nessun dato valido
  }

  brightness = constrain(eeprom_read_byte((uint8_t*)EE_BRIGHTNESS), 1, 10);
  mute = eeprom_read_byte((uint8_t*)EE_MUTE) != 0;
  channel = constrain(eeprom_read_byte((uint8_t*)EE_CHANNEL), 1, 3);

  channelTime[0] = (int16_t)eeprom_read_word((uint16_t*)EE_CH_A_TIME);
  channelTime[1] = (int16_t)eeprom_read_word((uint16_t*)EE_CH_B_TIME);
  channelTime[2] = (int16_t)eeprom_read_word((uint16_t*)EE_CH_C_TIME);

  for (int i = 0; i < 3; i++) {
    channelTime[i] = constrain(channelTime[i], 0, 9999);
  }

  channelsRelayActivated[0] = eeprom_read_byte((uint8_t*)EE_CH_A_RELAY) != 0;
  channelsRelayActivated[1] = eeprom_read_byte((uint8_t*)EE_CH_B_RELAY) != 0;
  channelsRelayActivated[2] = eeprom_read_byte((uint8_t*)EE_CH_C_RELAY) != 0;

  // Carica tempo del canale corrente
  timeSet = channelTime[channel - 1];
  appoTimeSet = timeSet;

  return true;
}

void saveSettings() {
  // eeprom_update scrive SOLO se il valore e' cambiato (preserva vita EEPROM)
  eeprom_update_byte((uint8_t*)EE_MAGIC, EEPROM_MAGIC);
  eeprom_update_byte((uint8_t*)EE_BRIGHTNESS, brightness);
  eeprom_update_byte((uint8_t*)EE_MUTE, mute ? 1 : 0);
  eeprom_update_byte((uint8_t*)EE_CHANNEL, channel);

  eeprom_update_word((uint16_t*)EE_CH_A_TIME, (uint16_t)channelTime[0]);
  eeprom_update_word((uint16_t*)EE_CH_B_TIME, (uint16_t)channelTime[1]);
  eeprom_update_word((uint16_t*)EE_CH_C_TIME, (uint16_t)channelTime[2]);

  eeprom_update_byte((uint8_t*)EE_CH_A_RELAY, channelsRelayActivated[0] ? 1 : 0);
  eeprom_update_byte((uint8_t*)EE_CH_B_RELAY, channelsRelayActivated[1] ? 1 : 0);
  eeprom_update_byte((uint8_t*)EE_CH_C_RELAY, channelsRelayActivated[2] ? 1 : 0);
}

#endif // STORAGE_H
