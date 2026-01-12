# KConfig Usage Quick Reference

## Accessing Configuration

### ESP-IDF Framework

Use `idf.py menuconfig` to configure:

```bash
# Open configuration menu
pio run -e esp32dev-idf -t menuconfig

# Or directly with idf.py (if ESP-IDF is installed)
idf.py menuconfig
```

### Arduino Framework

Edit `include/config.h` directly.

## Configuration Values in Code

All configuration values are accessible via the same `#define` names regardless of framework:

```cpp
#include "config.h"

// These work in both Arduino and ESP-IDF
int pin = TFT_MOSI;           // Display MOSI pin
int width = DISPLAY_WIDTH;    // Display width
bool test = TEST_MODE;         // Test mode flag
int delay = WIFI_RECONNECT_DELAY;  // WiFi reconnect delay
```

## Special Cases

### Cost Per Unit (ESP-IDF)

In ESP-IDF, `COST_PER_UNIT_DEFAULT` is stored as a string. To use it:

```cpp
#include "config.h"
#include <stdlib.h>  // for atof

#ifdef CONFIG_COST_PER_UNIT_DEFAULT
    // ESP-IDF: Parse from string
    float cost = atof(CONFIG_COST_PER_UNIT_DEFAULT);
#else
    // Arduino: Use numeric value directly
    float cost = COST_PER_UNIT_DEFAULT;
#endif
```

Or use a helper function:

```cpp
float get_cost_per_unit() {
    #ifdef CONFIG_COST_PER_UNIT_DEFAULT
        return atof(CONFIG_COST_PER_UNIT_DEFAULT);
    #else
        return COST_PER_UNIT_DEFAULT;
    #endif
}
```

## Configuration Categories

### Hardware Pins
- `TFT_MOSI`, `TFT_MISO`, `TFT_SCLK`, `TFT_CS`, `TFT_DC`, `TFT_RST`, `TFT_BL`
- `TOUCH_SCLK`, `TOUCH_MOSI`, `TOUCH_MISO`, `TOUCH_CS`, `TOUCH_IRQ`
- `FLOW_METER_PIN`
- `LED_R_PIN`, `LED_G_PIN`, `LED_B_PIN`
- `RFID_CS_PIN`, `RFID_RST_PIN`

### Display Settings
- `DISPLAY_WIDTH`, `DISPLAY_HEIGHT`, `DISPLAY_ROTATION`
- `TOUCH_THRESHOLD`

### System Settings
- `SERIAL_BAUD`
- `TEST_MODE` (0 or 1)

### WiFi Settings
- `WIFI_RECONNECT_DELAY`
- `USE_IMPROV_WIFI` (0 or 1)
- `IMPROV_WIFI_TIMEOUT_MS`
- `USE_SAVED_CREDENTIALS` (0 or 1)

### MQTT Settings
- `MQTT_PORT`
- `MQTT_CLIENT_ID_PREFIX` (string)
- `MQTT_TOPIC_PREFIX` (string)
- `MQTT_RECONNECT_DELAY`
- `MQTT_KEEPALIVE`

### Error Recovery
- `ENABLE_WATCHDOG` (0 or 1)
- `WATCHDOG_TIMEOUT_SEC`
- `MAX_CONSECUTIVE_ERRORS`
- `ERROR_RESET_DELAY_MS`

### Pouring Mode
- `COST_PER_UNIT_DEFAULT` (float in Arduino, string in ESP-IDF)
- `CURRENCY_SYMBOL` (string)

### Development
- `WOKWI_SIMULATOR` (0 or 1)

## Example: Reading Configuration

```cpp
#include "config.h"

void setup() {
    Serial.begin(SERIAL_BAUD);
    
    // Pin configuration
    pinMode(TFT_BL, OUTPUT);
    pinMode(FLOW_METER_PIN, INPUT_PULLUP);
    
    // Mode check
    if (TEST_MODE) {
        Serial.println("Running in TEST MODE");
    } else {
        Serial.println("Running in PRODUCTION MODE");
    }
    
    // WiFi settings
    Serial.printf("WiFi reconnect delay: %d ms\n", WIFI_RECONNECT_DELAY);
    
    // MQTT settings
    Serial.printf("MQTT port: %d\n", MQTT_PORT);
    Serial.printf("MQTT prefix: %s\n", MQTT_TOPIC_PREFIX);
}
```

## Verifying Configuration

### ESP-IDF

Check generated configuration:

```bash
# View sdkconfig file
cat sdkconfig | grep CONFIG_PRECISIONPOUR

# Or view in menuconfig
idf.py menuconfig
```

### Arduino

Check `include/config.h` directly.

## Troubleshooting

### Configuration not found

If you get "undefined reference" errors:

1. **ESP-IDF**: Ensure `sdkconfig` exists and was generated
2. **Arduino**: Check `include/config.h` has the define
3. **Both**: Clean and rebuild: `pio run -t clean`

### Wrong values

1. **ESP-IDF**: Re-run `menuconfig` and verify settings
2. **Arduino**: Check `include/config.h` values
3. **Both**: Clean build: `pio run -t clean`

### Framework detection

The code automatically detects framework:

```cpp
#ifdef CONFIG_TFT_MOSI
    // ESP-IDF detected
#else
    // Arduino detected
#endif
```

This happens automatically in `include/config.h` - you don't need to check this in your code.
