// KEYPAD_MGMT_H

#ifndef KEYPAD_MGMT_H
#define KEYPAD_MGMT_H

#include <Arduino.h>
#include <ArduinoSTL.h>
#include "variables.h"
#include "Keypad.h"
#include "variables.h"

// PROTOTYPES
void evaluateKey(int toneType, int duration);

// IMPLEMENTATIONS
// void keypadEvent(KeypadEvent key) {}
void evaluateKey()
{

  boolean isFocus = operationStatus.current.name == MODES_TEXTS[MODES_IDS::FOCUS];
  
  char key = ' ';

  if (keypad.getKeys()) { // Riempie un array con tutti i tasti premuti

    for (int ki = 0; ki < LIST_MAX; ki++) { // Scorro l'array
      if (keypad.key[ki].stateChanged) { // Only find keys that have changed state.

        switch (keypad.key[ki].kstate)
        { // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
        case HOLD:
        case PRESSED:
        case IDLE:
          return;
          break;

        case RELEASED:
          key = keypad.key[ki].kchar != NO_KEY ? keypad.key[ki].kchar : ' ';
          break;
        }
      }
    }

    int rawTime = 0;
    switch (key) {
    case 'A':
      channel = 1;
      showChannel(); /*init_timermode();*/
      break;
    case 'B':
      channel = 2;
      showChannel(); /*init_timermode();*/
      break;
    case 'C':
      channel = 3;
      showChannel(); /*init_timermode();*/
      break;
    case 'D': // reset
      // say_clr(); //M. TODO
      delay(250);
      // reset(); TODO
      break;

    case '*':

      if (isFocus) {
        brightness = brightness > 10 ? brightness - 10 : 10;
        sevenSegmentsDisplay.write(0x7A); // Brightness control command
        sevenSegmentsDisplay.write((byte)brightness);
        break;
      }

      switch (operationStatus.current.precisionType.id) {
      
        case PrecisionType::FSTOP:
          multSet = pow(2.0, (1.0 / precisionSet));
          rawTime = int(((float(timeSet) / multSet) * 10 + 5) / 10);
          timeSet = rawTime > 1 ? rawTime : 1;
          appoTimeSet = timeSet;
          showTime(timeSet);
          break;

        case PrecisionType::SECONDS:
        default:
          rawTime = timeSet - precisionSet * 10;
          timeSet = rawTime > 1 ? rawTime : 1;
          appoTimeSet = timeSet;
          showTime(timeSet);
          break;
        }
      break;

    case '#':

      if (isFocus) {
        brightness = brightness < 10 ? 10 : brightness + 10;
        sevenSegmentsDisplay.write(0x7A); // Brightness control command
        sevenSegmentsDisplay.write((byte)brightness);
        break;
      }
      
      switch (operationStatus.current.precisionType.id) {
       
        case PrecisionType::FSTOP:
          multSet = pow(2.0, (1.0 / precisionSet));
          rawTime = int(((float(timeSet) * multSet) * 10 + 5) / 10);
          timeSet = rawTime <= 9999 ? rawTime : 9999;
          appoTimeSet = timeSet;
          showTime(timeSet);
        break;

        case PrecisionType::SECONDS:
        default:
          rawTime = timeSet + precisionSet * 10;
          timeSet = rawTime > 1 ? rawTime : 1;
          appoTimeSet = timeSet;
          showTime(timeSet);
        break;

      }
      break;

    default:
      if (operationStatus.current.name == MODES_TEXTS[MODES_IDS::FOCUS]) { // setup audio on/off
        switch (key)
        {
        case '0':
          if (mute == true) {
            mute = false;
            emitSound(TONE_UP, 200);
            showSound();
          } else {
            mute = true;
            showMute();
          }
          break;
        case '1':
          channelsRelayActivated[channel] = channelsRelayActivated[channel] ? false : true;
          showRel();
          break;

        case '2':
          pushOrPullActMode = pushOrPullActMode == HIGH ? LOW : HIGH;
          showPushOrPullActMode();
          break;
        default:
          break;
        }
      } else {
        switch (nCifra)
        {
        case 0:
          timeSet = timeSet + (key - 48);
          nCifra++;
          break;
        case 1:
          timeSet = timeSet * 10 + (key - 48);
          nCifra++;
          break;
        case 2:
          timeSet = timeSet * 10 + (key - 48);
          nCifra++;
          break;
        case 3:
          timeSet = timeSet * 10 + (key - 48);
          nCifra++;
          break;
        default:
          // Non faccio nulla
          break;
        }
        appoTimeSet = timeSet;
        showTime(timeSet);
      }
    }
  }
}

#endif // KEYPAD_MGMT_H