# R080 TIMER MEGA - Photographic Enlarger Timer

## Description
This project implements an advanced digital timer for photographic enlargers, based on Arduino Mega 2560. The device offers various operating modes, including Focus, Countdown, and Test Strip, with precise controls for managing exposure times.

## Development Board

This project has been specifically developed for the **Arduino Mega 2560**.

### Main features of Arduino Mega 2560:
- Microcontroller: ATmega2560
- Operating voltage: 5V
- Digital I/O pins: 54 (of which 15 can be used as PWM outputs)
- Analog input pins: 16
- DC current per I/O pin: 20 mA
- DC current for 3.3V pin: 50 mA
- Flash Memory: 256 KB (of which 8 KB used by bootloader)
- SRAM: 8 KB
- EEPROM: 4 KB
- Clock Speed: 16 MHz

The choice of Arduino Mega 2560 was motivated by the need for a high number of I/O pins to manage all project components (display, encoder, keypad, etc.) and the greater memory capacity compared to other Arduino boards, which allows for implementing more complex functionalities.

## Hardware Configuration

### Components used:
1. Arduino Mega 2560
2. 16x2 RGB LCD Display (I2C)
3. 7-segment Display
4. 4x4 Numeric Keypad
5. 2x Rotary Encoders with button
6. Main Button
7. Buzzer
8. Relay for enlarger control

### Connections:

- **I2C LCD**:
  - SDA -> Pin 20
  - SCL -> Pin 21

- **7-segment Display**:
  - DIN -> Pin A2

- **4x4 Keypad**:
  - Rows: Pins 33, 31, 29, 27
  - Columns: Pins 41, 39, 37, 35

- **Mode Encoder**:
  - DT -> Pin 3
  - CLK -> Pin 4
  - SW -> Pin 17

- **Precision Encoder**:
  - DT -> Pin 18
  - CLK -> Pin 19
  - SW -> Pin 16

- **Main Button**: Pin 2

- **Buzzer**: Pin 49

- **Enlarger Relay**: Pin 47

## Software Configuration

### Development Environment
This project uses PlatformIO as the development environment. To set up the project:

- #### Using Visual Studio Code:
  1. Install the PlatformIO IDE extension for Visual Studio Code
  2. Clone this repository
  3. Open the project folder in Visual Studio Code
  4. Click on the PlatformIO icon in the sidebar
  5. Select "Build" to compile the project
  6. Select "Upload" to upload the firmware to Arduino Mega 2560

- #### Using PlatformIO Core:

  1. Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html)
  2. Clone this repository
  3. Open a command window in the project directory
  4. Run `pio run` to compile the project
  5. Run `pio run --target upload` to upload the firmware to Arduino Mega 2560

### Libraries used

- **EncoderButton**: Management of rotary encoders with integrated button
  - [Github](https://github.com/Stutchbury/EncoderButton)

- **Encoder**: Rotary encoder management
  - [Github](https://github.com/PaulStoffregen/Encoder)

- **Keypad**: Numeric keypad management
  - [Github](https://playground.arduino.cc/Code/Keypad/)
  
- **Waveshare_LCD1602_RGB**: Control of the RGB LCD display via I2C
  - [Wiki](https://www.waveshare.com/wiki/LCD1602_RGB_Module)

- **ArduinoSTL**: Implementation of some C++ standard library functionalities
  - [Github](https://github.com/mike-matera/ArduinoSTL)


## Contributing

Pull requests are welcome. 
For major changes, please open an issue first to discuss what you would like to change.

## License

[CC0 1.0 Universal](LICENSE)

