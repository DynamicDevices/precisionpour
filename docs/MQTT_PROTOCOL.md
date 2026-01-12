# MQTT Communication Protocol

This document describes the MQTT communication protocol used by the PrecisionPour firmware.

## Topic Structure

All MQTT topics follow this pattern:
```
precisionpour/{CHIP_ID}/commands
precisionpour/{CHIP_ID}/commands/paid
```

Where `{CHIP_ID}` is the ESP32's MAC address (e.g., `0070072D9200`).

### Example Topics
- `precisionpour/0070072D9200/commands` - General commands
- `precisionpour/0070072D9200/commands/paid` - Payment/pour initiation

## Commands

### "paid" Command

**Topic**: `precisionpour/{CHIP_ID}/commands/paid`

**Format**: JSON

**Purpose**: Initiates a pouring session with payment parameters.

**Payload**:
```json
{
  "id": "unique_payment_id",
  "cost_per_ml": 0.005,
  "max_ml": 500,
  "currency": "GBP"
}
```

**Fields**:
- `id` (string, required): Unique identifier for this pour transaction
- `cost_per_ml` (float, required): Cost per milliliter (e.g., 0.005 = £0.005/ml or $0.005/ml)
- `max_ml` (integer, required): Maximum milliliters allowed for this pour
- `currency` (string, optional): Currency code - "GBP" (British Pounds) or "USD" (US Dollars). Defaults to "GBP" if not specified.

**Response**:
- Device switches to pouring screen
- Flow meter starts tracking volume
- UI displays real-time flow rate, volume, and cost
- Pouring stops automatically when `max_ml` is reached

**Example**:
```bash
mosquitto_pub -h mqtt.dynamicdevices.co.uk \
  -t "precisionpour/0070072D9200/commands/paid" \
  -m '{"id":"tx_12345","cost_per_ml":0.005,"max_ml":500,"currency":"GBP"}'
```

### "start_pour" Command

**Topic**: `precisionpour/{CHIP_ID}/commands`

**Format**: Plain text or JSON

**Purpose**: Switch to pouring screen (without payment parameters).

**Payload**:
- Plain text: `start_pour`
- JSON: `{"action":"start_pour"}`

**Response**:
- Device switches to pouring screen
- Flow meter starts tracking (no cost limits)

### "stop_pour" Command

**Topic**: `precisionpour/{CHIP_ID}/commands`

**Format**: Plain text or JSON

**Purpose**: Switch back to production screen (QR code).

**Payload**:
- Plain text: `stop_pour`
- JSON: `{"action":"stop_pour"}`

**Response**:
- Device switches to production screen
- Flow meter continues tracking (not reset)

### "cost" Command (Legacy)

**Topic**: `precisionpour/{CHIP_ID}/commands`

**Format**: Plain text or JSON

**Purpose**: Update cost per unit (legacy format, use "paid" command for new implementations).

**Payload**:
- Plain text: `cost:5.50` (cost per liter)
- JSON: `{"cost":5.50}`

**Note**: This command is deprecated. Use the "paid" command instead.

## Currency Support

The firmware supports two currency codes:
- **"GBP"**: British Pounds - displays as "GBP " (text prefix, as £ symbol not in font)
- **"USD"**: US Dollars - displays as "$"

Currency is parsed from the JSON payload and converted to the appropriate display symbol. If an unsupported currency is provided, the default currency symbol from `config.h` is used.

## Screen Switching

### Automatic Switching
- Receiving "paid" command → Switches to pouring screen
- Receiving "stop_pour" command → Switches to production screen

### Manual Switching
- User can tap anywhere on the pouring screen to return to production screen
- Touch event triggers `LV_EVENT_CLICKED` on the screen object

## Flow Meter Integration

When pouring mode is active:
- Flow rate is displayed in **mL/min** (milliliters per minute)
- Volume is displayed in **ml** (milliliters)
- Cost is calculated as: `total_cost = volume_ml × cost_per_ml`
- Maximum volume limit is enforced (pouring stops at `max_ml`)

## Debugging

### Serial Output
All MQTT messages are logged to serial:
```
[MQTT] Received message on topic: precisionpour/0070072D9200/commands/paid
[MQTT] Payload: {"id":"tx_12345","cost_per_ml":0.005,"max_ml":500,"currency":"GBP"}
[MQTT] Paid command received:
  ID: tx_12345
  Cost per ml: 0.0050
  Max ml: 500
  Currency: GBP
```

### Error Handling
- Invalid JSON → Error logged, command ignored
- Missing required fields → Error logged, command ignored
- Invalid currency code → Default currency used, warning logged

## Testing

### Using mosquitto_pub
```bash
# Test "paid" command
mosquitto_pub -h mqtt.dynamicdevices.co.uk \
  -t "precisionpour/0070072D9200/commands/paid" \
  -m '{"id":"test_001","cost_per_ml":0.005,"max_ml":500,"currency":"GBP"}'

# Test "start_pour" command
mosquitto_pub -h mqtt.dynamicdevices.co.uk \
  -t "precisionpour/0070072D9200/commands" \
  -m "start_pour"

# Test "stop_pour" command
mosquitto_pub -h mqtt.dynamicdevices.co.uk \
  -t "precisionpour/0070072D9200/commands" \
  -m "stop_pour"
```

### Using MQTT Client Libraries
```python
import paho.mqtt.client as mqtt

client = mqtt.Client()
client.connect("mqtt.dynamicdevices.co.uk", 1883, 60)

# Send paid command
payload = {
    "id": "test_001",
    "cost_per_ml": 0.005,
    "max_ml": 500,
    "currency": "GBP"
}
client.publish("precisionpour/0070072D9200/commands/paid", json.dumps(payload))
```

## Future Enhancements

Potential additions:
- Volume-based pour completion notifications
- Flow rate alerts (too fast/slow)
- Pour history and statistics
- Multi-currency support expansion
- Pour cancellation commands
- Real-time flow data publishing
