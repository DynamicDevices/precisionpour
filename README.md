# PrecisionPour ESP32 Firmware

Firmware for ESP32-based touchscreen display system with flow meter monitoring, WiFi connectivity, and MQTT communication for the PrecisionPour beverage dispensing system.

## Development Environment Options

### Option 1: PlatformIO (Recommended) ⭐

**Pros:**
- Modern, professional development environment
- Integrated with VS Code
- Excellent library management
- Cross-platform support
- Built-in serial monitor and debugging tools

**Setup:**
1. Install VS Code
2. Install PlatformIO IDE extension
3. Open this project folder in VS Code
4. PlatformIO will automatically detect and set up the environment

**Usage:**
```bash
# Build the project
pio run

# Upload to ESP32
pio run -t upload

# Monitor serial output
pio device monitor
```

### Option 2: ESP-IDF (Official Framework)

**Pros:**
- Official Espressif development framework
- Full control over ESP32 features
- Advanced debugging capabilities
- Production-ready

**Setup:**
```bash
# Install ESP-IDF (Linux)
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32

# Source the environment
. ~/esp/esp-idf/export.sh
```

### Option 3: Arduino IDE

**Pros:**
- Simple and beginner-friendly
- Large community and examples
- Quick prototyping

**Setup:**
1. Install Arduino IDE
2. Add ESP32 board support:
   - File > Preferences > Additional Board Manager URLs
   - Add: `https://dl.espressif.com/dl/package_esp32_index.json`
3. Tools > Board > Boards Manager > Search "ESP32" > Install
4. Install libraries: TFT_eSPI, LVGL (if needed)

## Project Structure

```
precisionpour/
├── platformio.ini          # PlatformIO configuration
├── src/                    # Source files (.cpp)
│   └── main.cpp            # Main firmware entry point
├── include/                # Header files (.h, .hpp)
│   ├── config.h            # Hardware pin definitions
│   ├── display.h           # Display driver interface
│   └── touch.h             # Touch controller interface
├── lib/                    # Custom libraries (not in PlatformIO registry)
├── test/                   # Unit tests
├── data/                   # Filesystem data (SPIFFS/LittleFS)
├── docs/                   # Documentation
│   ├── SETUP_GUIDE.md      # PlatformIO setup instructions
│   ├── QUICK_REFERENCE.md  # Command reference
│   └── HARDWARE_SETUP.md   # Hardware configuration guide
└── README.md               # This file
```

### Directory Usage

- **`src/`** - All `.cpp` source files. PlatformIO automatically compiles these.
- **`include/`** - Header files. Automatically added to include path.
- **`lib/`** - Custom libraries not available via PlatformIO's library manager.
- **`test/`** - Unit tests. Run with `pio test`.
- **`data/`** - Files to upload to ESP32 filesystem. Upload with `pio run -t uploadfs`.
- **`docs/`** - Project documentation and guides.

## Hardware Configuration

### Display: 2.8" ESP32-32E Display Module
- **Display driver**: ILI9341 (240x320 TFT, landscape 320x240)
- **Touch controller**: XPT2046 (resistive touchscreen)
- **LCD SPI pins**: GPIO13 (MOSI), GPIO12 (MISO), GPIO14 (SCLK), GPIO15 (CS), GPIO2 (DC), GPIO4 (RST), GPIO21 (BL)
- **Touch SPI pins**: GPIO25 (SCLK), GPIO32 (MOSI), GPIO39 (MISO), GPIO33 (CS), GPIO36 (IRQ)
- **Note**: Touch screen uses a **separate SPI bus** from the LCD
- Pin connections defined in `include/config.h`
- Full pinout documentation: [docs/HARDWARE_SETUP.md](docs/HARDWARE_SETUP.md)

### Flow Meter
- **YF-S201 Hall Effect Flow Sensor**
- Flow rate range: 1-30 L/min
- Accuracy: ±10%
- Pulses per liter: 450
- Connected to GPIO 25 (interrupt-capable pin)
- **Note**: GPIO25 is also used for TOUCH_SCLK - see pin conflicts in [hardware docs](docs/HARDWARE_SETUP.md)
- Wiring:
  - Red wire → 5V
  - Black wire → GND
  - Yellow wire → GPIO 25

### Network Connectivity
- WiFi connection with auto-reconnect
- MQTT client for cloud communication
- Device-specific MQTT topics based on ESP32 MAC address

## Features

### UI Framework: LVGL
- Modern widget-based UI framework
- Touch input support
- Smooth animations and transitions
- Low memory footprint
- Hardware-accelerated rendering

### Production Mode UI
- PrecisionPour branded splashscreen with progress bar
- QR code for payment (device-specific URL)
- WiFi connection status icon (bottom left)
- Communication activity icon (bottom right)
- Logo and branding

### Flow Meter Integration
- Real-time flow rate measurement (L/min)
- Total volume tracking (liters)
- Interrupt-based pulse counting
- Automatic flow detection

### Network Features
- WiFi connection with auto-reconnect
- MQTT client for cloud communication
- Device-specific MQTT topics
- Activity monitoring and status indicators

### Project Structure

```
precisionpour/
├── src/
│   ├── main.cpp              # Main firmware entry point
│   ├── lvgl_display.cpp      # Display driver integration
│   ├── lvgl_touch.cpp        # Touch controller integration
│   ├── splashscreen.cpp      # Splashscreen with progress bar
│   ├── production_mode_ui.cpp # Production UI implementation
│   ├── test_mode_ui.cpp      # Test mode UI
│   ├── wifi_manager.cpp      # WiFi connection management
│   ├── mqtt_client.cpp       # MQTT client implementation
│   └── flow_meter.cpp        # Flow meter reading and calculations
├── include/
│   ├── config.h              # Hardware pin definitions
│   ├── lv_conf.h            # LVGL configuration
│   ├── lvgl_display.h       # Display driver interface
│   ├── lvgl_touch.h         # Touch driver interface
│   ├── splashscreen.h       # Splashscreen interface
│   ├── production_mode_ui.h # Production UI interface
│   ├── wifi_manager.h       # WiFi manager interface
│   ├── mqtt_client.h        # MQTT client interface
│   ├── flow_meter.h         # Flow meter interface
│   └── secrets.h            # WiFi/MQTT credentials (gitignored)
└── lib/
    └── TFT_eSPI_Config/     # TFT_eSPI display configuration
        └── User_Setup.h
```

## Quick Start

1. **Install PlatformIO**: See [docs/SETUP_GUIDE.md](docs/SETUP_GUIDE.md)
2. **Configure Credentials**: Copy `include/secrets.h.example` to `include/secrets.h` and add your WiFi/MQTT credentials
3. **Hardware Setup**: Connect display, touch, and flow meter according to pin definitions in `include/config.h`
4. **Build & Upload**: Use PlatformIO commands (see [docs/QUICK_REFERENCE.md](docs/QUICK_REFERENCE.md))

## Configuration

### WiFi and MQTT Setup
1. Copy `include/secrets.h.example` to `include/secrets.h`
2. Edit `include/secrets.h` and add your credentials:
   ```cpp
   #define WIFI_SSID "YourWiFiNetwork"
   #define WIFI_PASSWORD "YourPassword"
   #define MQTT_SERVER "mqtt.example.com"
   ```
3. The `secrets.h` file is gitignored and will not be committed

### Operating Modes
- **Production Mode** (`TEST_MODE = 0`): PrecisionPour branded UI with QR code
- **Test Mode** (`TEST_MODE = 1`): Hardware testing interface

Set in `include/config.h`:
```cpp
#define TEST_MODE 0  // 0 = Production, 1 = Test
```

## API Usage

### Flow Meter
```cpp
// Get current flow rate
float flow_rate = flow_meter_get_flow_rate_lpm();  // Liters per minute

// Get total volume
float volume = flow_meter_get_total_volume_liters();  // Total liters

// Reset volume counter
flow_meter_reset_volume();
```

### WiFi
```cpp
// Check connection status
bool connected = wifi_manager_is_connected();

// Get IP address
String ip = wifi_manager_get_ip();

// Get MAC address
String mac = wifi_manager_get_mac_address();
```

### MQTT
```cpp
// Publish message
mqtt_client_publish("topic", "message");

// Check connection
bool connected = mqtt_client_is_connected();

// Check for activity
bool active = mqtt_client_has_activity();
```

## Documentation

- **[Setup Guide](docs/SETUP_GUIDE.md)** - Detailed PlatformIO installation instructions
- **[Quick Reference](docs/QUICK_REFERENCE.md)** - Common commands and shortcuts
- **[Hardware Setup](docs/HARDWARE_SETUP.md)** - Hardware configuration guide
- **[Flow Meter Guide](docs/FLOW_METER.md)** - YF-S201 flow sensor integration details
- **[Project Structure](docs/PROJECT_STRUCTURE.md)** - Code organization and file structure