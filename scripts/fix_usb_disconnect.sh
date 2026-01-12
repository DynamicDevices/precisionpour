#!/bin/bash
#
# Copyright (c) 2026 Dynamic Devices Ltd
# All rights reserved.
#
# Script to prevent USB disconnection issues with ESP32
# Fixes common issues that cause ESP32 devices to disconnect during upload
#

set -euo pipefail

echo "=== Fixing USB Disconnection Issues ==="
echo ""

# 1. Kill any processes using the USB port
echo "1. Killing processes using USB ports..."
for port in /dev/ttyUSB*; do
    if [ -e "$port" ]; then
        echo "   Checking $port..."
        lsof "$port" 2>/dev/null | grep -v COMMAND | awk '{print $2}' | xargs -r kill -9 2>/dev/null || true
    fi
done

# 2. Disable USB autosuspend for all USB devices
echo "2. Disabling USB autosuspend..."
for usb_dev in /sys/bus/usb/devices/*/power/control; do
    if [ -f "$usb_dev" ]; then
        echo "on" | sudo tee "$usb_dev" > /dev/null 2>&1 || true
    fi
done

# 3. Set USB port permissions
echo "3. Setting USB port permissions..."
sudo chmod 666 /dev/ttyUSB* 2>/dev/null || true

# 4. Disable ModemManager (if installed) - it can interfere with serial ports
echo "4. Stopping ModemManager (if running)..."
if systemctl is-active --quiet ModemManager 2>/dev/null; then
    sudo systemctl stop ModemManager
    echo "   ModemManager stopped"
else
    echo "   ModemManager not running"
fi

# 5. Disable brltty (Braille terminal) - can interfere with USB serial
echo "5. Stopping brltty (if running)..."
if systemctl is-active --quiet brltty 2>/dev/null; then
    sudo systemctl stop brltty
    echo "   brltty stopped"
else
    echo "   brltty not running"
fi

echo ""
echo "=== Done ==="
echo "USB disconnection fixes applied."
echo ""
echo "If your ESP32 still disconnects, try:"
echo "  - Using a different USB cable (data cable, not power-only)"
echo "  - Using a USB 2.0 port instead of USB 3.0"
echo "  - Using a powered USB hub"
echo "  - Checking if the ESP32 board has a power LED"
echo "  - Running: ./fix_brltty.sh (if brltty is causing issues)"