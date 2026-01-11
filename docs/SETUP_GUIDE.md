# PlatformIO Setup Guide

## Step 1: Install PlatformIO Extension

1. Open VS Code
2. Press `Ctrl+Shift+X` (or `Cmd+Shift+X` on Mac) to open Extensions
3. Search for "PlatformIO IDE"
4. Click "Install" on the official PlatformIO IDE extension by PlatformIO
5. Wait for installation to complete (this may take a few minutes)

## Step 2: Open the Project

1. In VS Code, go to `File > Open Folder`
2. Navigate to `/home/ajlennon/data_drive/dd/precisionpour`
3. Click "Open"

## Step 3: PlatformIO Auto-Setup

When you open the project:
- PlatformIO will automatically detect `platformio.ini`
- It will download the ESP32 platform and toolchain (first time only, ~200MB)
- A progress bar will appear in the bottom status bar
- Wait for "PlatformIO: Ready" in the status bar

## Step 4: Verify Installation

1. Look for the PlatformIO icon (ant head) in the left sidebar
2. Click it to open the PlatformIO Home
3. You should see your project listed

## Step 5: Build Your First Firmware

1. Click the PlatformIO icon in the bottom status bar (or press `Ctrl+Alt+B`)
2. Or use the terminal: `pio run`
3. First build will take longer as it downloads dependencies

## Step 6: Upload to ESP32

1. Connect your ESP32 via USB
2. Check which port it's on: `pio device list` or look in PlatformIO sidebar
3. Update `platformio.ini` if needed with `upload_port = /dev/ttyUSB0` (or your port)
4. Click the upload button (→) in the status bar, or run `pio run -t upload`

## Step 7: Monitor Serial Output

1. Click the serial monitor icon (plug) in the status bar
2. Or run: `pio device monitor`
3. You should see "ESP32 Touchscreen Display Firmware" message

## Troubleshooting

### PlatformIO extension not installing
- Make sure you have internet connection
- Try restarting VS Code
- Check VS Code version (should be recent)

### Build errors
- Make sure ESP32 is selected: Check bottom status bar shows "esp32dev"
- Try: `pio run -t clean` then rebuild

### Upload errors
- Check USB cable and connection
- Verify port: `ls /dev/ttyUSB*` or `ls /dev/ttyACM*`
- You may need to add your user to dialout group:
  ```bash
  sudo usermod -a -G dialout $USER
  # Then log out and back in
  ```

### Permission denied on serial port
```bash
sudo chmod 666 /dev/ttyUSB0  # Replace with your port
# Or add user to dialout group (see above)
```

## Next Steps

Once PlatformIO is set up:
1. Identify your display hardware specifications
2. Update pin definitions in `src/main.cpp`
3. Add appropriate libraries to `platformio.ini`
4. Start coding!
