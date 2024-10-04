# R080 TIMER MEGA - Photographic Enlarger Timer

## General Overview

This project implements an advanced digital timer for photographic enlargers, based on Arduino Mega. The device offers several operating modes, including Focus, Countdown, and Test Strip, with precise controls for exposure time management.

### Main Components

0. Arduino Mega 2560
1. 16x2 LCD Display (Waveshare 1602 RGB LCD)
2. 7-segment Display (Sparkfun Serial 7 Segment Display)
3. Numeric Keypad
4. Rotary Encoder for mode selection
5. Rotary Encoder for precision adjustment
6. Main button for start/stop (arcade button)
7. Relay for enlarger control

### Operating Modes

1. **Focus**: Unlimited time counting.
2. **Countdown**: Countdown from a preset time.
3. **Test Strip**: Incremental exposures for print tests.

## Software Architecture

The project is organized into several header files that manage specific functionalities:

- `types.h`: Defines data structures and enumerations.
- `constants.h`: Contains global constants.
- `variables.h`: Declares global variables.
- `config.h`: Manages initial component configuration.
- `presentation.h`: Functions for LCD and 7-segment display.
- `keypad_mgmt.h`: Numeric keypad management.
- `encoder_mgmt.h`: Rotary encoder management.
- `running_mgmt.h`: Execution state and main button management.
- `operations.h`: Defines and manages available operations.
- `actions.h`: Implements specific actions for each mode.

## Execution Flow

1. **Initialization**:
   - Pin and hardware component configuration.
   - Initialization of available operations.

2. **Main Loop**:
   - Main button management (`manageMainButtonAction()`).
   - Evaluation of encoder and keypad inputs.
   - Execution of the current action based on the selected mode.

3. **Event Handling**:
   - Interrupts handle main button events.
   - Callback functions handle encoder events.

4. **Mode Change**:
   - The mode encoder modifies `operationStatus.current`.
   - LCD presentation is updated accordingly.

5. **Action Execution**:
   - Each mode (Focus, Countdown, Test Strip) has its own execution function.
   - Actions are started/stopped by the main button.

## Implementation Details

### Main Button Management

The main button uses an interrupt system for maximum responsiveness:
```cpp
void mainButtonHandler() {
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > DEBOUNCE_TIME) {
        mainBtnEventOccurred = true;
        lastInterruptTime = interruptTime;
    }
}
```

The `manageMainButtonAction()` function in `running_mgmt.h` handles the action to be taken based on the current state.

### Operation Structure

Operations are defined as objects of the `Operation` class:

```cpp
class Operation {
    public:
        String name;
        byte id;
        ModalityTypeStatus modalityTypeStatus;
        PrecisionTypeStatus precisionTypeStatus;
        PrecisionStatus precisionStatus;
        ChannelInfo channelsInfo[CHANNELS_IDS::SIZE];
        void (showPresentation)();
        void (switchModality)(ModalityTypeStatus );
        void (switchPrecisionType)(PrecisionTypeStatus );
        void (startedAction)();
// ...
}
```

### Encoder and Button Management

Encoders and buttons are managed through the `Encoder` and `EncoderButton` libraries:

```cpp
// Encoder definitions
Encoder encoderMode(ENC_MODE_DT_PIN, ENC_MODE_CLK_PIN);
Encoder encoderPrecision(ENC_PRECISION_DT_PIN, ENC_PRECISION_CLK_PIN);

// Encoder button definitions
EncoderButton encoderModeButton = EncoderButton(ENC_MODE_SW_PIN);
EncoderButton encoderPrecisionButton = EncoderButton(ENC_PRECISION_SW_PIN);

// Mode encoder management
void evaluateEncoderMod() {
  long newEncoderPosition = encoderMode.read() >> 2;
  int timerModeIndex = (newEncoderPosition % 3 + 3) % 3;
  setOperationStatus(timerModeIndex);
  showModalityType();
}

// Precision encoder management
void evaluateEncoderPrec() {
  long newEncoderPosition = encoderPrecision.read() >> 2;
  int precisionIndex = (newEncoderPosition % 11 + 11) % 11;
  setOperationPrecision(precisionIndex);
  showPrecision();
}

// Mode button management
void onModeButtonEvent(EncoderButton& encoderButton) {
  operationStatus.current.switchModality(&operationStatus.current.modalityTypeStatus);
}

// Precision button management
void onPrecisionButtonEvent(EncoderButton& encoderButton) {
  operationStatus.current.switchPrecisionType(&operationStatus.current.precisionTypeStatus);
}

// Event handler registration for buttons
void registerClickHandler() {
  encoderModeButton.setClickHandler(onModeButtonEvent);
  encoderPrecisionButton.setClickHandler(onPrecisionButtonEvent);
}

// Button state update
void evaluateButtons() {
  encoderModeButton.update();
  encoderPrecisionButton.update();
}
```

Encoders are used to select the operating mode (Focus, Countdown, Test Strip) and the precision level. Encoder buttons allow changing the precision mode (F-stop or Seconds) and the Test Strip mode (Continuous or Step-by-Step).

- The mode encoder (`encoderMode`) selects between the three main operating modes.
- The precision encoder (`encoderPrecision`) adjusts the precision level on a scale of 11 values.
- The mode button (`encoderModeButton`) changes the Test Strip mode between Continuous and Step-by-Step.
- The precision button (`encoderPrecisionButton`) toggles between F-stop and Seconds precision modes.

The `evaluateEncoderMod()` and `evaluateEncoderPrec()` functions read the encoder positions and update the application state accordingly. The `onModeButtonEvent()` and `onPrecisionButtonEvent()` functions handle button events, modifying the respective modes.

The `registerClickHandler()` function associates event handlers with buttons, while `evaluateButtons()` updates the button states in each main loop cycle.

### Specific Actions

Actions for each mode are implemented in `actions.h`. For example, for Test Strip:

```cpp
void testStrip() {
    if (!isActionStarted && isFirstTimePressed) {
        initTestStrip();
        isFirstTimePressed = false;
    }
// ... logic for time increment and relay control
}
```

## Maintenance Considerations

- The modular structure facilitates the addition of new features.
- The use of callback functions allows easy customization of behavior.
- State management through `operationStatus` centralizes control of the application flow.

## Possible Improvements

1. Implement a logging system for debugging.
2. Add the ability to save and load custom configurations.
3. Improve error handling and implement a recovery system.
4. Optimize memory usage, especially for constant strings.