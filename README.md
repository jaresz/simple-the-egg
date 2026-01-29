# ESP32C3 OLED Project

## Overview
This project is designed to display environmental parameter: temperature, humidity, and air pressure on a 0.42 inch OLED with a resolution of 72x40 pixels.
It uses ESP32-C3 microcontroller with integrated, tiny 0.42 inch OLED 72x40 display and AHT20 + BMP280 sensor module.
It is developed using PlatformIO and Arduino framework.

## Features
- **ESP32 supermini** - ESP32-C3 OLED development board with integrated 0.42 inch OLED disaplay module,  WiFi, Bluetooth and ceramic antenna **
- **Development Environment**: PlatformIO
- **Language**: C++
- **Applications**: IoT, data visualization

## Getting Started

### Prerequisites
Ensure you have the following installed:
- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO IDE](https://platformio.org/install/ide)
- A compatible ESP32-C3 board with 0.42"/72x40 pixel OLED display module
- AHT20 + BMP280 sensor module

### Installation
1. Clone this repository:
   ```bash
   git clone git@github.com:jaresz/simple-the-egg.git
   ```
2. Open the project in PlatformIO IDE.
3. Connect your ESP32-C3 board to your computer.
4. Build and upload the project:
   ```bash
   pio run --target upload
   ```

### File Structure
- `platformio.ini`: Configuration file for PlatformIO.
- `src/main.cpp`: Main application code.
- `lib/`: Custom libraries.


## Hardware Details

### ESP32-C3 OLED Development Board
This project uses the ESP32-C3 OLED development board, which features:
- **Integrated OLED Display**: 0.42 inch monochromatic screen with a resolution of 72x40 pixels.
- **Connectivity**: Wi-Fi and Bluetooth capabilities.
- **Antenna**: Ceramic antenna for improved signal strength.
- **Compact Design**: Supermini form factor.

### Sensor Module: AHT20 + BMP280
The AHT20 + BMP280 module is used for measuring temperature, humidity, and air pressure. It combines:
- **AHT20**: High-accuracy temperature and humidity sensor.
- **BMP280**: Barometric pressure sensor.

### Wiring Instructions
The AHT20 + BMP280 module shares the same I2C bus as the integrated OLED screen. The connections are as follows:
<img src="images/bb.png" alt="Wiring: ESP32-C3 OLED development board and Sensor Module: AHT20 + BMP280" style="">
| Module Pin | ESP32-C3 Pin |
|------------|--------------|
| VCC        | 3.3V         |
| GND        | GND          |
| SCL        | GPIO6        |
| SDA        | GPIO5        |

- **SCL (Clock)**: Connect to GPIO6, which is also used by the OLED display.
- **SDA (Data)**: Connect to GPIO5, which is also used by the OLED display.

The I2C bus allows multiple devices to communicate over the same data and clock lines. 

## Usage
1. Power on the ESP32-C3 board.
2. The OLED display will show the output.
3. Modify `src/main.cpp` to customize the behavior.

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

