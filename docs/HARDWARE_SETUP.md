# Hardware Setup Guide

## Hardware: 2.8" ESP32-32E Display Module

This firmware is configured for the **2.8-inch ESP32-32E Display Module** which integrates:
- ESP32-WROOM-32E microcontroller
- 2.8" TFT LCD (240x320 resolution, ILI9341 driver)
- Resistive touchscreen (XPT2046 controller)
- Onboard peripherals (MicroSD, Audio, RGB LED, etc.)

**Documentation Source**: [lcdwiki.com - 2.8inch ESP32-32E Display](https://www.lcdwiki.com/2.8inch_ESP32-32E-7789)

## Pin Configuration

### LCD Display Pins (SPI Bus)

The LCD uses a dedicated SPI bus:

| Pin Function | GPIO | Description |
|-------------|------|-------------|
| **TFT_CS** | GPIO15 | LCD screen selection control signal (low level effective) |
| **TFT_DC** | GPIO2 | LCD command/data selection (high=data, low=command) |
| **TFT_RST** | GPIO4 | LCD reset control signal (low level reset, shared with ESP32 EN pin) |
| **TFT_SCLK** | GPIO14 | LCD SPI bus clock signal |
| **TFT_MOSI** | GPIO13 | LCD SPI bus write data signal |
| **TFT_MISO** | GPIO12 | LCD SPI bus read data signal |
| **TFT_BL** | GPIO21 | LCD backlight control (high=on, low=off) |

### Touch Screen Pins (Separate SPI Bus)

**Important**: The touch screen uses its **own dedicated SPI bus**, separate from the LCD SPI bus.

| Pin Function | GPIO | Description |
|-------------|------|-------------|
| **TOUCH_CS** | GPIO33 | Touch chip selection control signal (low level effective) |
| **TOUCH_IRQ** | GPIO36 | Touch interrupt signal (low level = touch event) |
| **TOUCH_SCLK** | GPIO25 | Touch SPI bus clock signal |
| **TOUCH_MOSI** | GPIO32 | Touch SPI bus write data signal |
| **TOUCH_MISO** | GPIO39 | Touch SPI bus read data signal (input only) |

### Flow Meter (YF-S201)

| Pin Function | GPIO | Description |
|-------------|------|-------------|
| **FLOW_METER_PIN** | GPIO26 | Flow meter interrupt signal (Hall effect sensor output) |

**Note**: Changed from GPIO25 to GPIO26 to avoid conflict with TOUCH_SCLK. GPIO26 is interrupt-capable and was previously used for Audio DAC (can be repurposed).

### Available Peripherals

#### MicroSD Card
| Pin Function | GPIO | Description |
|-------------|------|-------------|
| **SD_CS** | GPIO5 | SD card select signal (low level effective) |
| **SD_MOSI** | GPIO23 | SD card SPI bus write data (shared with LCD SPI) |
| **SD_SCLK** | GPIO18 | SD card SPI bus clock (shared with LCD SPI) |
| **SD_MISO** | GPIO19 | SD card SPI bus read data (shared with LCD SPI) |

**Note**: MicroSD shares SPI pins with LCD. Cannot be used simultaneously without careful management.

#### Audio
| Pin Function | GPIO | Description |
|-------------|------|-------------|
| **AUDIO_EN** | GPIO4 | Audio enable signal (low level enable, high level disable) |
| **AUDIO_DAC** | GPIO26 | Audio signal DAC output signal |

**Note**: GPIO4 conflicts with TFT_RST, so audio enable cannot be used simultaneously.

#### RGB LED
| Pin Function | GPIO | Description |
|-------------|------|-------------|
| **LED_R** | GPIO22 | Red LED control (common anode: low=on, high=off) |
| **LED_G** | GPIO16 | Green LED control (common anode: low=on, high=off) |
| **LED_B** | GPIO17 | Blue LED control (common anode: low=on, high=off) |

#### Buttons
| Pin Function | GPIO | Description |
|-------------|------|-------------|
| **BOOT** | GPIO0 | Download mode select button (press and hold to power on, then release to enter download mode) |
| **EN** | EN | ESP32 reset button (low level reset, shared with LCD reset) |

#### Serial Port
| Pin Function | GPIO | Description |
|-------------|------|-------------|
| **RXD0** | GPIO3 | Serial port receiving signal (can be used as GPIO if serial not used) |
| **TXD0** | GPIO1 | Serial port transmit signal (can be used as GPIO if serial not used) |

#### Battery Monitoring
| Pin Function | GPIO | Description |
|-------------|------|-------------|
| **BAT_ADC** | GPIO34 | Battery voltage ADC input (input only, ADC1_CH6) |

#### SPI Peripheral
| Pin Function | GPIO | Description |
|-------------|------|-------------|
| **SPI_CS** | GPIO27 | SPI peripheral chip selection (low level effective, can be GPIO if not used) |
| **SPI_SCLK** | GPIO18 | SPI bus clock (shared with MicroSD/LCD, currently used for LCD) |
| **SPI_MISO** | GPIO19 | SPI bus read data (shared with MicroSD/LCD, currently used for LCD) |
| **SPI_MOSI** | GPIO23 | SPI bus write data (shared with MicroSD/LCD, currently used for LCD) |

#### Not Connected (Input Only)
| Pin Function | GPIO | Description |
|-------------|------|-------------|
| **NC** | GPIO35 | Not connected, can only be used as input IO (ADC1_CH7) |

## Pin Conflicts and Notes

### Current Pin Usage
- **GPIO4**: TFT_RST (conflicts with AUDIO_EN)
- **GPIO18, GPIO19, GPIO23**: LCD SPI (shared with MicroSD/SPI peripheral)
- **GPIO25**: TOUCH_SCLK (touch SPI clock)
- **GPIO26**: FLOW_METER_PIN (flow meter interrupt, was Audio DAC)
- **GPIO34, GPIO35, GPIO36, GPIO39**: Input-only pins (ADC or touch)

### Available for Future Use
- **GPIO27**: SPI peripheral CS (can be GPIO if not used)
- **GPIO32**: Touch MOSI (currently used, but could be repurposed if touch not needed)

## Flow Meter Setup (YF-S201)

### Wiring
- **Red wire** → 5V (ESP32 5V pin)
- **Black wire** → GND (ESP32 GND)
- **Yellow wire** → GPIO 26 (signal output, interrupt-capable pin)

**Note**: Changed from GPIO25 to GPIO26 to avoid conflict with TOUCH_SCLK. GPIO26 is interrupt-capable and available for use.

### Specifications
- Flow rate range: 1-30 liters per minute
- Accuracy: ±10%
- Pulses per liter: 450
- Operating voltage: 4.5V to 18V DC
- Maximum current: 15mA at 5V

### How It Works
The YF-S201 uses a Hall effect sensor to detect a spinning rotor. Each rotation generates a pulse. The flow rate is calculated from the pulse frequency:
- Flow Rate (L/min) = Pulse Frequency (Hz) / 7.5

The firmware automatically:
- Counts pulses via hardware interrupt
- Calculates flow rate every second
- Tracks total volume
- Provides API functions to read values

## Configuration Files

### `include/config.h`
Contains all pin definitions and hardware configuration:
- Display pins
- Touch pins
- Flow meter pin
- WiFi/MQTT settings
- Operating mode (Test/Production)

### `lib/TFT_eSPI_Config/User_Setup.h`
TFT_eSPI library configuration:
- Display driver (ILI9341)
- Pin assignments
- SPI frequency settings
- Display resolution

## Testing Your Setup

1. **Upload firmware** with correct pin configuration
2. **Check serial output** for initialization messages:
   ```bash
   pio device monitor
   ```
3. **Verify display**:
   - Splashscreen should appear
   - Backlight should turn on
   - Main screen should display (logo, QR code, etc.)
4. **Test touch**:
   - Touch screen should respond
   - Check serial output for touch coordinates
5. **Test flow meter**:
   - Connect flow meter
   - Check serial output for flow readings
   - Verify flow rate calculations

## Troubleshooting

### Display Issues
- **Display shows nothing**: 
  - Check backlight pin (GPIO21) - verify it's turning on
  - Verify SPI connections (GPIO13, GPIO12, GPIO14)
  - Check TFT_CS pin (GPIO15)
  - Verify power supply is adequate
  
- **Display shows garbage/corrupted image**:
  - Check SPI speed settings
  - Verify all SPI connections are correct
  - Check display driver selection (ILI9341)

- **Backlight not working**:
  - Verify GPIO21 is configured correctly
  - Check TFT_BL pin connection
  - Verify backlight control logic (HIGH = on)

### Touch Issues
- **Touch not working**:
  - Verify touch uses separate SPI bus (GPIO25, GPIO32, GPIO39)
  - Check TOUCH_CS pin (GPIO33)
  - Verify TOUCH_IRQ pin (GPIO36)
  - Check serial output for touch initialization messages
  - Touch may need calibration

- **Touch coordinates incorrect**:
  - Check touch calibration values in code
  - Verify touch SPI communication is working
  - Check serial output for raw touch values

### Flow Meter Issues
- **Flow meter shows no reading**:
  - Check wiring (Red→5V, Black→GND, Yellow→GPIO26)
  - Verify GPIO26 is interrupt-capable
  - Ensure liquid is actually flowing
  - Check serial output for pulse count

- **Flow readings inaccurate**:
  - Check for air bubbles in flow path
  - Verify sensor is properly installed
  - Check calibration factor (7.5 pulses per L/min)

### Pin Conflict Issues
- **GPIO4 conflict**: Used for TFT_RST, conflicts with AUDIO_EN
  - Solution: Cannot use audio enable if display reset is needed
- **SPI sharing**: LCD, MicroSD, and SPI peripheral share GPIO18, GPIO19, GPIO23
  - Solution: Cannot use all simultaneously without careful management
- **GPIO26**: Now used for FLOW_METER_PIN (was Audio DAC, can be repurposed)

## References

- [LCD Wiki - 2.8inch ESP32-32E Display](https://www.lcdwiki.com/2.8inch_ESP32-32E-7789)
- [ESP32-32E Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.pdf)
- [YF-S201 Flow Sensor Datasheet](https://www.seeedstudio.com/blog/2019/08/27/water-flow-sensor-with-arduino/)
