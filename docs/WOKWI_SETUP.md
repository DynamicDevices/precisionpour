# Wokwi Simulator Setup

## Quick Start

1. **Install Wokwi VS Code Extension**:
   - Open VS Code
   - Press `Ctrl+Shift+X` (or `Cmd+Shift+X` on Mac)
   - Search for "Wokwi"
   - Install "Wokwi for VS Code" by Wokwi

2. **Activate License** (first time only):
   - Press `F1` to open Command Palette
   - Type "Wokwi: Request a new License"
   - Follow the prompts (free license available)

3. **Build the Firmware**:
   ```bash
   pio run
   ```

4. **Start Simulation**:
   - Press `F1` → "Wokwi: Start Simulator"
   - Or click the Wokwi icon in the VS Code sidebar
   - Or right-click on `.wokwi/diagram.json` → "Open With" → "Wokwi Diagram Editor"

## Configuration Files

### `wokwi.toml`
Main configuration file that points to the compiled firmware:
- `firmware`: Path to `.bin` file
- `elf`: Path to `.elf` file (for debugging)
- Defines parts (ESP32, display, touch controller)

### `.wokwi/diagram.json`
Circuit diagram with pin connections:
- ESP32 DevKit V1
- ILI9341 TFT Display (320x240)
- XPT2046 Touch Controller

### `.wokwi/config.json`
VS Code extension configuration:
- Board type
- Main sketch file
- Library dependencies

## Pin Connections

All pins match `include/config.h`:

**Display (ILI9341)**:
- MOSI: GPIO 23
- MISO: GPIO 19
- SCLK: GPIO 18
- CS: GPIO 5
- DC: GPIO 2
- RST: GPIO 4
- BL (Backlight): GPIO 15

**Touch (XPT2046)**:
- Shares SPI bus with display
- CS: GPIO 21
- IRQ: GPIO 22 (optional)

## Usage Tips

1. **Build Before Simulating**:
   - Always run `pio run` first to generate firmware files
   - Wokwi needs the `.bin` and `.elf` files

2. **Serial Monitor**:
   - Serial output appears in Wokwi's serial monitor
   - Baud rate: 115200 (matches `SERIAL_BAUD` in config.h)

3. **Display Simulation**:
   - Wokwi shows a visual representation of the display
   - Click and drag on the display to simulate touch input
   - Display updates in real-time as your code runs

4. **Limitations**:
   - Display simulation is basic - may not perfectly match hardware
   - Some LVGL features may not render exactly as on real hardware
   - Touch simulation works but may feel different from real hardware
   - Always test on real hardware for final validation

## Troubleshooting

**"Firmware not found"**:
- Run `pio run` to build the firmware first
- Check that `.pio/build/esp32dev/firmware.bin` exists

**"Display not showing"**:
- Verify pin connections in `.wokwi/diagram.json`
- Check that your code initializes the display correctly
- Look for errors in the serial monitor

**"Touch not working"**:
- Verify touch controller is properly initialized in code
- Check that touch CS pin (GPIO 21) is correct
- Try clicking directly on the display in the simulator

## Next Steps

1. Test your splashscreen with progress bar
2. Verify UI elements render correctly
3. Test touch interactions
4. Debug serial output
5. When ready, test on real hardware!

## Resources

- [Wokwi Documentation](https://docs.wokwi.com)
- [Wokwi ESP32 Guide](https://docs.wokwi.com/guides/esp32)
- [Wokwi VS Code Extension](https://marketplace.visualstudio.com/items?itemName=wokwi.wokwi-vscode)
