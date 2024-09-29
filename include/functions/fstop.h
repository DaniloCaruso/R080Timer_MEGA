#ifndef FSTOP_H
#define FSTOP_H

#include <Arduino.h>
#include <math.h>

struct FStopState {
    int baseTime;     // Decisecondi, impostato da keypad
    int stopOffset;   // Stop dal base (-n a +n)

    void reset() {
        baseTime = 0;
        stopOffset = 0;
    }

    void setBase(int time) {
        baseTime = time;
        stopOffset = 0;
    }

    // precisionValue e' il denominatore: 1=full, 2=half, 3=third, ecc.
    int getCalculatedTime(int precisionValue) const {
        if (baseTime <= 0 || precisionValue <= 0) return baseTime;
        float multiplier = pow(2.0, (float)stopOffset / (float)precisionValue);
        int result = (int)((float)baseTime * multiplier + 0.5f);
        return constrain(result, 1, 9999);
    }

    void stepUp() { stopOffset++; }
    void stepDown() { stopOffset--; }

    // Formatta offset per LCD: "+02", "-01", " 00"
    void formatOffset(char* buf) const {
        if (stopOffset > 0) {
            int clamped = stopOffset < 99 ? stopOffset : 99;
            sprintf(buf, "+%02d", clamped);
        } else if (stopOffset < 0) {
            int clamped = stopOffset > -99 ? stopOffset : -99;
            sprintf(buf, "%03d", clamped);
        } else {
            buf[0] = ' '; buf[1] = '0'; buf[2] = '0'; buf[3] = '\0';
        }
    }
};

#endif // FSTOP_H
