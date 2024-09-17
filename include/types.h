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

struct ModalityTypeStruct
{
  ModalityType id;
  String value;
  ModalityTypeStruct(ModalityType i, String v) : id(i), value(v) {}
};

struct PrecisionTypeStruct
{
  PrecisionType id;
  String value;
  PrecisionTypeStruct(PrecisionType i, String v) : id(i), value(v) {}
};

struct OperationActions
{
  void (*toStartAction)();
  void (*startedAction)();
  
  OperationActions(
    void (*toStart)(),
    void (*started)()
  ) :
    toStartAction(toStart),
    startedAction(started) {}
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
  static void noSwitchMod(ModalityTypeStruct *type) { }
  static void noSwitchPrec(PrecisionTypeStruct *type) { }


public:
  String name;
  ModalityTypeStruct modalityType = ModalityTypeStruct(ModalityType::CONTINOUS, "CO");
  PrecisionTypeStruct precisionType = PrecisionTypeStruct(PrecisionType::SECONDS, "S");
  PrecisionValueString precision = PRECISION_MAP_LISTS[precisionType.id][0];
  void (*showPresentation)();
  void (*switchModality)(ModalityTypeStruct *);
  void (*switchPrecisionType)(PrecisionTypeStruct *);
  void (*toStartAction)();
  void (*startedAction)();
  void setPrecision(int index) {
    // Trova la lista di precisioni associata al tipo di precisione attuale
    if (PRECISION_MAP_LISTS.count(precisionType.id) > 0) {
        std::vector<PrecisionValueString>& precisionList = PRECISION_MAP_LISTS[precisionType.id];
        
        // Verifica se l'indice è valido
        if (index >= 0 && index < static_cast<signed int>(precisionList.size())) {
            // Imposta il valore di precisione
            precision = precisionList[index];
        } else {
            Serial.println("Indice di precisione non valido.");
        }
    } else {
        Serial.println("Tipo di precisione non trovato nella mappa.");
    }
  }


  // Costruttore di default
  Operation() : 
    name(""),
    modalityType(ModalityTypeStruct(NO_MODALITY, "")),
    precisionType(PrecisionTypeStruct(NO_PRECISION, "")),
    showPresentation(noActionOrPresentation),
    switchModality(noSwitchMod),
    switchPrecisionType(noSwitchPrec),
    toStartAction(noActionOrPresentation),
    startedAction(noActionOrPresentation) {}
  
  // COSTRUTTORE NO MODALITA NO PRECISIONE SENZA ACTION (PER SOLI MESSAGGI A VIDEO)
  Operation(
      String n,
      void (*showPres)()
  ) : 
    name(n),
    modalityType(ModalityTypeStruct(NO_MODALITY, "")),
    precisionType(PrecisionTypeStruct(NO_PRECISION, "")),
    showPresentation(showPres),
    switchModality(noSwitchMod),
    switchPrecisionType(noSwitchPrec),
    toStartAction(noActionOrPresentation),
    startedAction(noActionOrPresentation) {}

  // COSTRUTTORE NO MODALITA NO PRECISIONE CON ACTIONs (FOCUS)
  Operation(
    String n,
    void (*showPres)(),
    void (*toStart)(),
    void (*started)()
  ) : 
    name(n),
    modalityType(ModalityTypeStruct(NO_MODALITY, "")),
    precisionType(PrecisionTypeStruct(NO_PRECISION, "")),
    showPresentation(showPres),
    switchModality(noSwitchMod),
    switchPrecisionType(noSwitchPrec),
    toStartAction(toStart),
    startedAction(started) {}
  
  // COSTRUTTORE NO MODALITA CON PRECISIONE E ACTION (COUNTDOWN)
  Operation(
    String n,
    PrecisionTypeStruct p,
    void (*showPres)(),
    void (*switchPrec)(PrecisionTypeStruct *),
    void (*toStart)(),
    void (*started)()
  ) : 
    name(n),
    modalityType(ModalityTypeStruct(NO_MODALITY, "")),
    precisionType(p),
    showPresentation(showPres),
    switchModality(noSwitchMod),
    switchPrecisionType(switchPrec),
    toStartAction(toStart),
    startedAction(started) {}

  // COSTRUTTORE NO PRECISIONE CON MODALITA E ACTION (INUTILE? NON USATO)
  Operation(
    String n,
    ModalityTypeStruct m,
    void (*showPres)(),
    void (*switchMod)(ModalityTypeStruct *),
    void (*toStart)(),
    void (*started)()
  ) : 
    name(n),
    modalityType(m),
    precisionType(PrecisionTypeStruct(NO_PRECISION, "")),
    showPresentation(showPres),
    switchModality(switchMod),
    switchPrecisionType(noSwitchPrec),
    toStartAction(toStart),
    startedAction(started) {}

  // COSTRUTTORE CON PRECISIONE MODALITA E  ACTION (PROVINO SCALARE)
  Operation(
    String n,
    ModalityTypeStruct m,
    PrecisionTypeStruct p,
    void (*showPres)(),
    void (*switchMod)(ModalityTypeStruct *),
    void (*switchPrec)(PrecisionTypeStruct *),
    void (*toStart)(),
    void (*started)()
  ) : 
    name(n),
    modalityType(m),
    precisionType(p),
    showPresentation(showPres),
    switchModality(switchMod),
    switchPrecisionType(switchPrec),
    toStartAction(toStart),
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
