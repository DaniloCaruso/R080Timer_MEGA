// KEYPAD_MGMT_H

#ifndef KEYPAD_MGMT_H
#define KEYPAD_MGMT_H

#include <Arduino.h>
#include "variables.h"
#include "Keypad.h"

// PROTOTYPES
void evaluateKey();
void switchToChannel(byte ch);
void updateTimeFromFstop();

// IMPLEMENTATIONS

void switchToChannel(byte ch) {
    // Salva tempo canale corrente
    channelTime[channel - 1] = timeSet;
    // Cambia canale
    channel = ch;
    // Carica tempo canale selezionato
    timeSet = channelTime[channel - 1];
    appoTimeSet = timeSet;
    nCifra = 0;
    // Aggiorna F-stop state col nuovo tempo base
    fstopState.setBase(timeSet);
    showChannel();
    showTime(timeSet);
    saveSettings();
}

void updateTimeFromFstop() {
    int precisionSet = operationStatus.current.precisionStatus.current.value;
    timeSet = fstopState.getCalculatedTime(precisionSet);
    appoTimeSet = timeSet;
    channelTime[channel - 1] = timeSet;
    showTime(timeSet);
    showPrecision();
    showFstopOffset();
    saveSettings();
}

void evaluateKey() {

    // FOCUS e' l'unico modo con NO_PRECISION; evita pointer-compare con
    // MODES_TEXTS (ora array di puntatori in PROGMEM).
    boolean isFocus = operationStatus.current.precisionTypeStatus.current.id == NO_PRECISION;
    char key = ' ';
    int rawTime = 0;
    int precisionSet = operationStatus.current.precisionStatus.current.value;

    if (keypad.getKeys()) {

        for (int ki = 0; ki < LIST_MAX; ki++) {
            if (keypad.key[ki].stateChanged) {

                switch (keypad.key[ki].kstate) {

                    case HOLD: {
                        char keyHolded = keypad.key[ki].kchar != NO_KEY ? keypad.key[ki].kchar : ' ';
                        switch (keyHolded) {
                            case '*':
                                if (mute) {
                                    mute = false;
                                    emitSound(TONE_UP, 200);
                                    showSound();
                                } else {
                                    mute = true;
                                    showMute();
                                }
                                saveSettings();
                                break;

                            case '#':
                                channelsRelayActivated[channel - 1] = !channelsRelayActivated[channel - 1];
                                showRel();
                                saveSettings();
                                break;

                            default:
                                break;
                        }
                    }
                    case PRESSED:
                    case IDLE:
                        return;

                    case RELEASED: {
                        key = keypad.key[ki].kchar != NO_KEY ? keypad.key[ki].kchar : ' ';
                        switch (key) {

            case 'A':
                switchToChannel(1);
                break;

            case 'B':
                switchToChannel(2);
                break;

            case 'C':
                switchToChannel(3);
                break;

            case 'D': // reset canale corrente
                channelTime[channel - 1] = 0;
                fstopState.reset();
                resetTime();
                saveSettings();
                break;

            case '*':
                if (isFocus) {
                    brightness = brightness > 1 ? brightness - 1 : 1;
                    sevenSegmentsDisplay.write(0x7A);
                    sevenSegmentsDisplay.write((byte)brightness);
                    saveSettings();
                    break;
                }

                // Navigazione F-stop o secondi
                switch (operationStatus.current.precisionTypeStatus.current.id) {
                    case PrecisionType::FSTOP:
                        // F-Stop: naviga indietro dal base time
                        if (fstopState.baseTime <= 0) fstopState.setBase(timeSet > 0 ? timeSet : 1);
                        fstopState.stepDown();
                        updateTimeFromFstop();
                        break;

                    case PrecisionType::SECONDS:
                    default:
                        rawTime = timeSet - precisionSet * 10;
                        timeSet = rawTime >= 1 ? rawTime : 1;
                        rawTime = 0;
                        appoTimeSet = timeSet;
                        channelTime[channel - 1] = timeSet;
                        showTime(timeSet);
                        saveSettings();
                        break;
                }
                break;

            case '#':
                if (isFocus) {
                    brightness = brightness < 10 ? brightness + 1 : 10;
                    sevenSegmentsDisplay.write(0x7A);
                    sevenSegmentsDisplay.write((byte)brightness);
                    saveSettings();
                    break;
                }

                // Navigazione F-stop o secondi
                switch (operationStatus.current.precisionTypeStatus.current.id) {
                    case PrecisionType::FSTOP:
                        // F-Stop: naviga avanti dal base time
                        if (fstopState.baseTime <= 0) fstopState.setBase(timeSet > 0 ? timeSet : 1);
                        fstopState.stepUp();
                        updateTimeFromFstop();
                        break;

                    case PrecisionType::SECONDS:
                    default:
                        rawTime = timeSet + precisionSet * 10;
                        timeSet = rawTime <= 9999 ? rawTime : 9999;
                        rawTime = 0;
                        appoTimeSet = timeSet;
                        channelTime[channel - 1] = timeSet;
                        showTime(timeSet);
                        saveSettings();
                        break;
                }
                break;

            default:
                if (!isFocus) {
                    if (key < '0' || key > '9') break;

                    int digitValue = key - '0';
                    int newTimeSet = timeSet;

                    switch (nCifra) {
                        case 0:
                            newTimeSet = digitValue;
                            break;
                        case 1:
                        case 2:
                        case 3:
                            newTimeSet = timeSet * 10 + digitValue;
                            break;
                        default:
                            break;
                    }

                    newTimeSet = constrain(newTimeSet, 0, 9999);
                    timeSet = newTimeSet;
                    if (nCifra < 3) nCifra++;
                    appoTimeSet = timeSet;
                    channelTime[channel - 1] = timeSet;
                    // Imposta come nuovo base per F-stop
                    fstopState.setBase(timeSet);
                    showTime(timeSet);
                    saveSettings();
                }
        } }
                }
            }
        }
    }
}

#endif // KEYPAD_MGMT_H
