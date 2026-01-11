#!/bin/bash
# Script to prevent USB disconnection issues with ESP32

echo "=== Fixing USB Disconnection Issues ==="

# 1. Kill any processes using the USB port
echo "1. Killing processes using USB ports..."
for port in /dev/ttyUSB*; do
    if [ -e "$port" ]; then
        echo "Checking $port..."
        lsof "$port" 2>/dev/null | grep -v COMMAND | awk '{print $2}' | xargs -r kill -9 2>/dev/null
    fi
done

# 2. Disable USB autosuspend for CH341 devices
echo "2. Disabling USB autosuspend..."
for usb_dev in /sys/bus/usb/devices/*/power/control; do
    if [ -f "$usb_dev" ]; then
        echo "on" | sudo tee "$usb_dev" > /dev/null 2>&1
    fi
done

# 3. Set USB port permissions
echo "3. Setting USB port permissions..."
sudo chmod 666 /dev/ttyUSB* 2>/dev/null || true

# 4. Disable ModemManager (if installed) - it can interfere with serial ports
echo "4. Stopping ModemManager (if running)..."
sudo systemctl stop ModemManager 2>/dev/null || echo "ModemManager not running"

# 5. Disable brltty (Braille terminal) - can interfere with USB serial
echo "5. Stopping brltty (if running)..."
sudo systemctl stop brltty 2>/dev/null || echo "brltty not running"

echo ""
echo "=== Done ==="
echo "Now try connecting your ESP32 device."
echo "If it still disconnects, try:"
echo "  - Using a different USB cable (data cable, not power-only)"
echo "  - Using a USB 2.0 port instead of USB 3.0"
echo "  - Using a powered USB hub"
echo "  - Checking if the ESP32 board has a power LED"
