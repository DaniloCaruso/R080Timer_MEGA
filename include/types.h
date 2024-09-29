#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include <avr/pgmspace.h>

typedef enum {
  FOCUS,
  COUNTDOWN,
  TEST_STRIP
} MODES_IDS;

typedef enum {
  CONTINOUS,
  STEP_BY_STEP,
  NO_MODALITY
} ModalityType;

typedef enum {
  FSTOP,
  SECONDS,
  NO_PRECISION
} PrecisionType;

// `str` punta a una stringa PROGMEM di 8 caratteri (+'\0'). Lettura via
// strcpy_P in keyBuffer prima di passare al rendering LCD.
struct PrecisionValueString {
    uint8_t     value;
    const char* str;
};

struct PrecisionStatus {
    PrecisionValueString current;
    PrecisionValueString last;
};

struct ModalityTypeStruct {
  ModalityType id;
  const char*  value;
  ModalityTypeStruct(ModalityType i, const char* v) : id(i), value(v) {}
  ModalityTypeStruct() : id(NO_MODALITY), value("") {}
};

struct ModalityTypeStatus {
  ModalityTypeStruct current;
  ModalityTypeStruct last;
};

struct PrecisionTypeStruct {
  PrecisionType id;
  const char*   value;
  PrecisionTypeStruct(PrecisionType i, const char* v) : id(i), value(v) {}
  PrecisionTypeStruct() : id(NO_PRECISION), value("") {}
};

struct PrecisionTypeStatus {
  PrecisionTypeStruct current;
  PrecisionTypeStruct last;
};

// Array costanti al posto di std::map<std::vector>
const int PRECISION_COUNT = 11;

// Stringhe precisioni in PROGMEM (flash): ciascuna 8 caratteri + '\0',
// allineate sulla riga 1 cols 0..7 dell'LCD. showPrecision legge via
// strcpy_P in keyBuffer.
const char FSTOP_STR_0[]  PROGMEM = "Pr:1/1  ";
const char FSTOP_STR_1[]  PROGMEM = "Pr:1/2  ";
const char FSTOP_STR_2[]  PROGMEM = "Pr:1/3  ";
const char FSTOP_STR_3[]  PROGMEM = "Pr:1/4  ";
const char FSTOP_STR_4[]  PROGMEM = "Pr:1/6  ";
const char FSTOP_STR_5[]  PROGMEM = "Pr:1/8  ";
const char FSTOP_STR_6[]  PROGMEM = "Pr:1/12 ";
const char FSTOP_STR_7[]  PROGMEM = "Pr:1/16 ";
const char FSTOP_STR_8[]  PROGMEM = "Pr:1/24 ";
const char FSTOP_STR_9[]  PROGMEM = "Pr:1/32 ";
const char FSTOP_STR_10[] PROGMEM = "Pr:1/64 ";

const PrecisionValueString FSTOP_PRECISIONS[PRECISION_COUNT] = {
  {   1, FSTOP_STR_0 },
  {   2, FSTOP_STR_1 },
  {   3, FSTOP_STR_2 },
  {   4, FSTOP_STR_3 },
  {   6, FSTOP_STR_4 },
  {   8, FSTOP_STR_5 },
  {  12, FSTOP_STR_6 },
  {  16, FSTOP_STR_7 },
  {  24, FSTOP_STR_8 },
  {  32, FSTOP_STR_9 },
  {  64, FSTOP_STR_10 }
};

const char SEC_STR_0[]  PROGMEM = "Pr: 1s  ";
const char SEC_STR_1[]  PROGMEM = "Pr: 2s  ";
const char SEC_STR_2[]  PROGMEM = "Pr: 3s  ";
const char SEC_STR_3[]  PROGMEM = "Pr: 4s  ";
const char SEC_STR_4[]  PROGMEM = "Pr: 5s  ";
const char SEC_STR_5[]  PROGMEM = "Pr: 6s  ";
const char SEC_STR_6[]  PROGMEM = "Pr: 7s  ";
const char SEC_STR_7[]  PROGMEM = "Pr: 8s  ";
const char SEC_STR_8[]  PROGMEM = "Pr: 10s ";
const char SEC_STR_9[]  PROGMEM = "Pr: 15s ";
const char SEC_STR_10[] PROGMEM = "Pr: 20s ";

const PrecisionValueString SECONDS_PRECISIONS[PRECISION_COUNT] = {
  {  1, SEC_STR_0 },
  {  2, SEC_STR_1 },
  {  3, SEC_STR_2 },
  {  4, SEC_STR_3 },
  {  5, SEC_STR_4 },
  {  6, SEC_STR_5 },
  {  7, SEC_STR_6 },
  {  8, SEC_STR_7 },
  { 10, SEC_STR_8 },
  { 15, SEC_STR_9 },
  { 20, SEC_STR_10 }
};

class Operation {
private:
  static void noActionOrPresentation() {}
  static void noSwitchMod(ModalityTypeStatus *type) { }
  static void noSwitchPrec(PrecisionTypeStatus *type) { }

public:
  const char* name;
  ModalityTypeStatus modalityTypeStatus = { current: ModalityTypeStruct(ModalityType::CONTINOUS, "C"), last: {} };
  PrecisionTypeStatus precisionTypeStatus = { current: PrecisionTypeStruct(PrecisionType::SECONDS, "S"), last: {} };
  PrecisionStatus precisionStatus = { current: SECONDS_PRECISIONS[0], last: {} };
  void (*showPresentation)();
  void (*switchModality)(ModalityTypeStatus *);
  void (*switchPrecisionType)(PrecisionTypeStatus *);
  void (*startedAction)();

  void setPrecisionStatus(int index) {
      if (precisionTypeStatus.current.id == NO_PRECISION) { return; }

      precisionStatus.last = precisionStatus.current;
      if (precisionTypeStatus.current.id == FSTOP) {
        precisionStatus.current = FSTOP_PRECISIONS[index];
      } else {
        precisionStatus.current = SECONDS_PRECISIONS[index];
      }
    }

  // Costruttore di default
  Operation() :
    name(""),
    modalityTypeStatus({ current: ModalityTypeStruct(NO_MODALITY, ""), last: {} }),
    precisionTypeStatus({ current: PrecisionTypeStruct(NO_PRECISION, ""), last: {} }),
    showPresentation(noActionOrPresentation),
    switchModality(noSwitchMod),
    switchPrecisionType(noSwitchPrec),
    startedAction(noActionOrPresentation) {}

  // FOCUS: no modalita, no precisione, con action
  Operation(
    const char* n,
    void (*showPres)(),
    void (*started)()
  ) :
    name(n),
    modalityTypeStatus({ current: ModalityTypeStruct(NO_MODALITY, ""), last: {} }),
    precisionTypeStatus({ current: PrecisionTypeStruct(NO_PRECISION, ""), last: {} }),
    showPresentation(showPres),
    switchModality(noSwitchMod),
    switchPrecisionType(noSwitchPrec),
    startedAction(started) {}

  // COUNTDOWN: no modalita, con precisione e action
  Operation(
    const char* n,
    PrecisionTypeStruct p,
    void (*showPres)(),
    void (*switchPrec)(PrecisionTypeStatus *),
    void (*started)()
  ) :
    name(n),
    modalityTypeStatus({ current: ModalityTypeStruct(NO_MODALITY, ""), last: {} }),
    precisionTypeStatus({ current: p, last: {} }),
    showPresentation(showPres),
    switchModality(noSwitchMod),
    switchPrecisionType(switchPrec),
    startedAction(started) {}

  // TEST STRIP: con precisione, modalita e action
  Operation(
    const char* n,
    ModalityTypeStruct m,
    PrecisionTypeStruct p,
    void (*showPres)(),
    void (*switchMod)(ModalityTypeStatus *),
    void (*switchPrec)(PrecisionTypeStatus *),
    void (*started)()
  ) :
    name(n),
    modalityTypeStatus({ current: m, last: {}}),
    precisionTypeStatus({current: p, last: {}}),
    showPresentation(showPres),
    switchModality(switchMod),
    switchPrecisionType(switchPrec),
    startedAction(started) {}
};

class OperationStatus {
  public:
    Operation current;
    Operation last;

    OperationStatus():
      current(Operation()),
      last(Operation()) {}
};

#endif // TYPES_H
