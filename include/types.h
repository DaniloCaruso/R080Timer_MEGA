#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include <ArduinoSTL.h>

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
} Modality;

typedef enum
{
  FSTOP,
  SECONDS,
  NO_PRECISION
} Precision;

struct ModalityType
{
  Modality id;
  String value;
  ModalityType(Modality i, String v) : id(i), value(v) {}
};

struct PrecisionType
{
  Precision id;
  String value;
  PrecisionType(Precision i, String v) : id(i), value(v) {}
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

class Operation
{
private:
  static void noActionOrPresentation() {}
  static void noSwitchMod(ModalityType *type) { }
  static void noSwitchPrec(PrecisionType *type) { }

public:
  String name;
  ModalityType modalityType;
  PrecisionType precisionType;
  void (*showPresentation)();
  void (*switchModality)(ModalityType *);
  void (*switchPrecision)(PrecisionType *);
  void (*toStartAction)();
  void (*startedAction)();

  // Costruttore di default
  Operation() : 
    name(""),
    modalityType(ModalityType(NO_MODALITY, "")),
    precisionType(PrecisionType(NO_PRECISION, "")),
    showPresentation(noActionOrPresentation),
    switchModality(noSwitchMod),
    switchPrecision(noSwitchPrec),
    toStartAction(noActionOrPresentation),
    startedAction(noActionOrPresentation) {}
  
  // COSTRUTTORE NO MODALITA NO PRECISIONE SENZA ACTION (PER SOLI MESSAGGI A VIDEO)
  Operation(
      String n,
      void (*showPres)()
  ) : 
    name(n),
    modalityType(ModalityType(NO_MODALITY, "")),
    precisionType(PrecisionType(NO_PRECISION, "")),
    showPresentation(showPres),
    switchModality(noSwitchMod),
    switchPrecision(noSwitchPrec),
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
    modalityType(ModalityType(NO_MODALITY, "")),
    precisionType(PrecisionType(NO_PRECISION, "")),
    showPresentation(showPres),
    switchModality(noSwitchMod),
    switchPrecision(noSwitchPrec),
    toStartAction(toStart),
    startedAction(started) {}
  
  // COSTRUTTORE NO MODALITA CON PRECISIONE E ACTION (COUNTDOWN)
  Operation(
    String n,
    PrecisionType p,
    void (*showPres)(),
    void (*switchPrec)(PrecisionType *),
    void (*toStart)(),
    void (*started)()
  ) : 
    name(n),
    modalityType(ModalityType(NO_MODALITY, "")),
    precisionType(p),
    showPresentation(showPres),
    switchModality(noSwitchMod),
    switchPrecision(switchPrec),
    toStartAction(toStart),
    startedAction(started) {}

  // COSTRUTTORE NO PRECISIONE CON MODALITA E ACTION (INUTILE? NON USATO)
  Operation(
    String n,
    ModalityType m,
    void (*showPres)(),
    void (*switchMod)(ModalityType *),
    void (*toStart)(),
    void (*started)()
  ) : 
    name(n),
    modalityType(m),
    precisionType(PrecisionType(NO_PRECISION, "")),
    showPresentation(showPres),
    switchModality(switchMod),
    switchPrecision(noSwitchPrec),
    toStartAction(toStart),
    startedAction(started) {}

  // COSTRUTTORE CON PRECISIONE MODALITA E  ACTION (PROVINO SCALARE)
  Operation(
    String n,
    ModalityType m,
    PrecisionType p,
    void (*showPres)(),
    void (*switchMod)(ModalityType *),
    void (*switchPrec)(PrecisionType *),
    void (*toStart)(),
    void (*started)()
  ) : 
    name(n),
    modalityType(m),
    precisionType(p),
    showPresentation(showPres),
    switchModality(switchMod),
    switchPrecision(switchPrec),
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
