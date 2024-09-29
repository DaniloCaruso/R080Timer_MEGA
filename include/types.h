#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include <ArduinoSTL.h>
#include <map>
#include <vector>

typedef enum {
  FOCUS,
  COUNTDOWN,
  TEST_STRIP
} MODES_IDS;


typedef enum
{
  CONTINOUS,
  STEP_BY_STEP,
  NO_MODALITY
} ModalityType;

typedef enum
{
  FSTOP,
  SECONDS,
  NO_PRECISION
} PrecisionType;

struct PrecisionValueString {
    int value;
    String string;
};

struct PrecisionStatus {
    PrecisionValueString current;
    PrecisionValueString last;
};

struct ModalityTypeStruct
{
  ModalityType id;
  String value;
  ModalityTypeStruct(ModalityType i, String v) : id(i), value(v) {}
  ModalityTypeStruct() : id(), value() {}
};

struct ModalityTypeStatus {
  ModalityTypeStruct current;
  ModalityTypeStruct last;
};

struct PrecisionTypeStruct
{
  PrecisionType id;
  String value;
  PrecisionTypeStruct(PrecisionType i, String v) : id(i), value(v) {}
  PrecisionTypeStruct() : id(), value() {}
};

struct PrecisionTypeStatus {
  PrecisionTypeStruct current;
  PrecisionTypeStruct last;
};

struct OperationActions
{
  void (*startAction)();
  
  OperationActions(
    void (*start)()
  ) :
    startAction(start) {}
};

std::map<PrecisionType, std::vector<PrecisionValueString>> PRECISION_MAP_LISTS = {
  { 
    PrecisionType::FSTOP, { 
      {   1, "Pr:1/1  "}, 
      {   2, "Pr:1/2  "}, 
      {   3, "Pr:1/3  "}, 
      {   4, "Pr:1/4  "}, 
      {   6, "Pr:1/6  "}, 
      {   8, "Pr:1/8  "}, 
      {  12, "Pr:1/12 "}, 
      {  16, "Pr:1/16 "}, 
      {  24, "Pr:1/24 "}, 
      {  32, "Pr:1/32 "}, 
      {  64, "Pr:1/64 "}, 
    }
  },
  { 
    PrecisionType::SECONDS, { 
      {  1, "Pr: 1s  " },
      {  2, "Pr: 2s  " },
      {  3, "Pr: 3s  " },
      {  4, "Pr: 4s  " },
      {  5, "Pr: 5s  " },
      {  6, "Pr: 6s  " },
      {  7, "Pr: 7s  " },
      {  8, "Pr: 8s  " },
      { 10, "Pr: 10s " },
      { 15, "Pr: 15s " },
      { 20, "Pr: 20s " },
    } 
  }
};

class Operation
{
private:
  static void noActionOrPresentation() {}
  static void noSwitchMod(ModalityTypeStatus *type) { }
  static void noSwitchPrec(PrecisionTypeStatus *type) { }


public:
  String name;
  ModalityTypeStatus modalityTypeStatus = { current: ModalityTypeStruct(ModalityType::CONTINOUS, "C"), last: {} };
  PrecisionTypeStatus precisionTypeStatus = { current: PrecisionTypeStruct(PrecisionType::SECONDS, "S"), last: {} };
  PrecisionStatus precisionStatus = { current: PRECISION_MAP_LISTS[precisionTypeStatus.current.id][0], last: { }};
  void (*showPresentation)();
  void (*switchModality)(ModalityTypeStatus *);
  void (*switchPrecisionType)(PrecisionTypeStatus *);
  void (*startedAction)();

  void setPrecisionStatus(int index) {
      // Trova la lista di precisioni associata al tipo di precisione attuale
      // Serial.println("id di precisione: " + (String)precisionTypeStatus.current.id);
      
      if (precisionTypeStatus.current.id == NO_PRECISION) { return; };
      
      std::vector<PrecisionValueString>& precisionList = PRECISION_MAP_LISTS[precisionTypeStatus.current.id];  // Ottieni la lista associata alla precisione
      precisionStatus.last = precisionStatus.current;
      precisionStatus.current = precisionList[index];      
    };

  boolean isPrecisionChanged () {
    return !precisionStatus.last.value || precisionStatus.current.value != precisionStatus.last.value;
  };
  boolean isPrecisionTypeChanged() {
    return !precisionTypeStatus.last.value || precisionTypeStatus.current.value != precisionTypeStatus.last.value;
  };
  boolean isModalityTypeChanged() {
    return !modalityTypeStatus.last.value || modalityTypeStatus.current.value != modalityTypeStatus.last.value;
  };

  // Costruttore di default
  Operation() : 
    name(""),
    modalityTypeStatus({ current: ModalityTypeStruct(NO_MODALITY, ""), last: {} }),
    precisionTypeStatus({ current: PrecisionTypeStruct(NO_PRECISION, ""), last: {} }),
    showPresentation(noActionOrPresentation),
    switchModality(noSwitchMod),
    switchPrecisionType(noSwitchPrec),
    startedAction(noActionOrPresentation) {}
  
  // COSTRUTTORE NO MODALITA NO PRECISIONE SENZA ACTION (PER SOLI MESSAGGI A VIDEO)
  Operation(
      String n,
      void (*showPres)()
  ) : 
    name(n),
    modalityTypeStatus({ current: ModalityTypeStruct(NO_MODALITY, ""), last: {} }),
    precisionTypeStatus({ current: PrecisionTypeStruct(NO_PRECISION, ""), last: {} }),
    showPresentation(showPres),
    switchModality(noSwitchMod),
    switchPrecisionType(noSwitchPrec),
    startedAction(noActionOrPresentation) {}

  // COSTRUTTORE NO MODALITA NO PRECISIONE CON ACTIONs (FOCUS)
  Operation(
    String n,
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
  
  // COSTRUTTORE NO MODALITA CON PRECISIONE E ACTION (COUNTDOWN)
  Operation(
    String n,
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

  // COSTRUTTORE NO PRECISIONE CON MODALITA E ACTION (INUTILE? NON USATO)
  Operation(
    String n,
    ModalityTypeStruct m,
    void (*showPres)(),
    void (*switchMod)(ModalityTypeStatus *),
    void (*started)()
  ) : 
    name(n),
    modalityTypeStatus({ current: m, last: {} }),
    precisionTypeStatus({ current: PrecisionTypeStruct(NO_PRECISION, ""), last: {} }),
    showPresentation(showPres),
    switchModality(switchMod),
    switchPrecisionType(noSwitchPrec),
    startedAction(started) {}

  // COSTRUTTORE CON PRECISIONE MODALITA E  ACTION (PROVINO SCALARE)
  Operation(
    String n,
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

class OperationStatus
{
  public: 
    Operation current;
    Operation last;

    OperationStatus(): 
      current(Operation()),
      last (Operation()) {}  
};

#endif // TYPES_H
