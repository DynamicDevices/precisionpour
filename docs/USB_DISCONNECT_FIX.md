# Fixing USB Disconnection Issues

## Problem
The ESP32 device connects and immediately disconnects, showing messages like:
```
usb 3-3.3.3: USB disconnect, device number 52
ch341-uart ttyUSB2: ch341-uart converter now disconnected
```

## Root Causes

1. **brltty (Braille Terminal)** - Automatically grabs USB serial devices
2. **ModemManager** - Can interfere with serial ports
3. **USB Power Management** - Auto-suspend can disconnect devices
4. **Multiple processes accessing port** - Conflicts when multiple programs try to use the port

## Solutions Applied

### 1. Disable brltty (Braille Terminal)
```bash
sudo systemctl stop brltty
sudo systemctl disable brltty
```

### 2. Stop ModemManager (if installed)
```bash
sudo systemctl stop ModemManager
```

### 3. Disable USB Auto-Suspend
The fix script disables USB autosuspend for all USB devices.

### 4. Updated Upload Settings
Modified `platformio.ini` to use `--before=no_reset` to prevent automatic resets that can cause disconnections.

## Quick Fix Script

Run the fix script:
```bash
cd /home/ajlennon/data_drive/dd/precisionpour
sudo ./scripts/fix_usb_disconnect.sh
```

## Manual Steps

If the script doesn't work, try these manually:

1. **Kill processes using USB ports:**
```bash
sudo lsof /dev/ttyUSB* | grep -v COMMAND | awk '{print $2}' | xargs -r kill -9
```

2. **Disable USB autosuspend:**
```bash
for usb_dev in /sys/bus/usb/devices/*/power/control; do
    echo "on" | sudo tee "$usb_dev" > /dev/null
done
```

3. **Set proper permissions:**
```bash
sudo chmod 666 /dev/ttyUSB*
```

## Hardware Checks

If disconnection persists, check:

1. **USB Cable**: Use a high-quality data cable (not power-only)
2. **USB Port**: Try USB 2.0 port instead of USB 3.0
3. **Power Supply**: ESP32 may need more power - try powered USB hub
4. **Board Power LED**: Check if power LED stays on (indicates stable power)

## Testing Connection

After applying fixes:

1. **Unplug and replug** the ESP32
2. **Wait 2-3 seconds** for device to stabilize
3. **Check device appears:**
```bash
ls -la /dev/ttyUSB*
```

4. **Try upload:**
```bash
pio run -t upload
```

## Permanent Fix

To make these changes permanent:

1. **Disable brltty permanently:**
```bash
sudo systemctl disable brltty
```

2. **Create udev rule** (already created):
```bash
sudo cat /etc/udev/rules.d/99-esp32-ch341.rules
```

3. **Reboot** (may be needed after blacklisting ch341):
```bash
sudo reboot
```

## If Still Disconnecting

1. Check `dmesg` for errors:
```bash
dmesg | tail -50 | grep -E "(usb|tty|ch341)"
```

2. Try different USB port on your computer
3. Try different USB cable
4. Check ESP32 board for loose connections
5. Verify ESP32 board has adequate power supply
