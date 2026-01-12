# Flow Meter Integration Guide

## YF-S201 Hall Effect Flow Sensor

This document describes the flow meter integration in the PrecisionPour firmware.

## Hardware Specifications

- **Model**: YF-S201 High Precision Water Flow Sensor
- **Flow Rate Range**: 1 to 30 liters per minute
- **Accuracy**: ±10%
- **Pulses per Liter**: 450
- **Operating Voltage**: 4.5V to 18V DC
- **Maximum Current**: 15mA at 5V
- **Output Type**: 5V TTL pulses
- **Thread Size**: G1/2" (1/2 inch)

## Wiring

| Wire Color | Connection | ESP32 Pin |
|------------|------------|-----------|
| Red        | Power      | 5V        |
| Black      | Ground     | GND       |
| Yellow     | Signal     | GPIO 26   |

**Important**: Changed from GPIO25 to GPIO26 to avoid conflict with TOUCH_SCLK. GPIO26 is interrupt-capable (it is on ESP32).

## How It Works

1. **Physical Principle**: As liquid flows through the sensor, it spins an internal rotor equipped with magnets.

2. **Hall Effect Detection**: The Hall effect sensor detects magnetic field changes from the rotor's rotation, generating a pulse signal.

3. **Pulse Counting**: The firmware uses a hardware interrupt to count each pulse on the RISING edge.

4. **Flow Rate Calculation**: 
   - Formula: `Flow Rate (L/min) = Pulse Frequency (Hz) / 7.5`
   - Where: `Pulse Frequency = pulses per second`
   - Example: 75 Hz = 10 L/min

5. **Volume Calculation**:
   - Formula: `Volume (L) = Total Pulses / 450`
   - The firmware continuously accumulates volume as pulses are counted

## Software Implementation

### Initialization

The flow meter is initialized in `setup()`:

```cpp
flow_meter_init();
```

This function:
- Configures GPIO 26 as input with pull-up
- Attaches interrupt handler for RISING edge
- Initializes internal counters

### Update Loop

The flow meter must be updated in the main loop:

```cpp
flow_meter_update();
```

This function:
- Calculates flow rate every 1 second
- Updates total volume
- Handles debouncing (10ms minimum pulse spacing)
- Detects when flow stops (2 seconds of no pulses)

### API Functions

#### Get Current Flow Rate
```cpp
float flow_rate = flow_meter_get_flow_rate_lpm();
// Returns: Flow rate in liters per minute (0.0 if no flow)
```

#### Get Total Volume
```cpp
float volume = flow_meter_get_total_volume_liters();
// Returns: Total volume in liters since last reset
```

#### Reset Volume Counter
```cpp
flow_meter_reset_volume();
// Resets total volume to 0.0 and pulse count to 0
```

#### Get Raw Pulse Count
```cpp
unsigned long pulses = flow_meter_get_pulse_count();
// Returns: Total pulse count (for debugging)
```

## Interrupt Handler

The interrupt service routine (ISR) is optimized for speed:

```cpp
void IRAM_ATTR flow_meter_isr() {
    // Debounce: ignore pulses < 10ms apart
    // Increment pulse counter
}
```

**Important Notes**:
- ISR must be fast (no delays, no Serial prints)
- Uses `IRAM_ATTR` to keep code in IRAM for speed
- Debouncing prevents false readings from electrical noise

## Flow Rate Examples

| Flow Rate (L/min) | Pulse Frequency (Hz) | Pulses per Second |
|-------------------|----------------------|-------------------|
| 1                 | 7.5                  | 7-8               |
| 5                 | 37.5                 | 37-38             |
| 10                | 75                   | 75                |
| 20                | 150                  | 150               |
| 30                | 225                  | 225               |

## Accuracy Considerations

- **±10% accuracy**: Flow readings may vary by up to 10%
- **Minimum flow**: Sensor requires at least 1 L/min to register
- **Air bubbles**: Can cause false readings - ensure proper installation
- **Installation**: Sensor must be installed in the correct flow direction (arrow on sensor)

## Debugging

### Serial Output

When flow is detected, the firmware outputs debug information:

```
[Flow Meter] Flow: 5.23 L/min, Total: 0.125 L, Pulses: 56
```

### Common Issues

1. **No readings**: 
   - Check wiring (especially yellow signal wire)
   - Verify GPIO 25 is correct
   - Ensure liquid is actually flowing
   - Check serial output for initialization messages

2. **Inaccurate readings**:
   - Check for air bubbles in the flow
   - Verify sensor is installed correctly (direction matters)
   - Ensure flow rate is within 1-30 L/min range

3. **Erratic readings**:
   - May be electrical noise - check wiring quality
   - Ensure proper grounding
   - Check power supply stability

## Integration with UI

The flow meter readings can be displayed in the UI by calling:

```cpp
float flow_rate = flow_meter_get_flow_rate_lpm();
float volume = flow_meter_get_total_volume_liters();
```

These values can be updated in `production_mode_update()` or `test_mode_update()` to show real-time flow information on the display.

## Future Enhancements

Potential improvements:
- Flow rate averaging over multiple samples
- Flow rate history/trending
- Automatic pour detection (flow start/stop)
- Volume-based pour limits
- MQTT publishing of flow data
