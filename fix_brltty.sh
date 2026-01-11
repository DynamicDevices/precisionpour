#!/bin/bash
# Script to completely prevent brltty from claiming CH341 USB-to-serial devices

echo "=== Preventing brltty from claiming CH341 devices ==="

# 1. Stop and disable brltty
echo "1. Stopping and disabling brltty..."
sudo systemctl stop brltty 2>/dev/null || true
sudo systemctl disable brltty 2>/dev/null || true
sudo systemctl mask brltty 2>/dev/null || true

# 2. Blacklist brltty module
echo "2. Blacklisting brltty module..."
echo "blacklist brltty" | sudo tee /etc/modprobe.d/blacklist-brltty.conf > /dev/null

# 3. Remove brltty module if loaded
echo "3. Removing brltty module..."
sudo modprobe -r brltty 2>/dev/null || echo "   brltty module not loaded"

# 4. Create udev rule to prevent brltty from claiming CH341 devices
echo "4. Creating udev rule..."
sudo tee /etc/udev/rules.d/99-prevent-brltty-ch341.rules > /dev/null << 'EOF'
# Prevent brltty from claiming CH341 USB-to-serial devices
ACTION=="add", SUBSYSTEM=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", ENV{BRLTTY_BRAILLE_DRIVER}=""
KERNEL=="ttyUSB*", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", ENV{BRLTTY_BRAILLE_DRIVER}="", MODE="0666", GROUP="dialout"
EOF

# 5. Reload udev rules
echo "5. Reloading udev rules..."
sudo udevadm control --reload-rules
sudo udevadm trigger

echo ""
echo "=== Done ==="
echo "brltty has been prevented from claiming CH341 devices."
echo ""
echo "Now unplug and replug your ESP32 device."
echo "It should stay connected as /dev/ttyUSB* without disconnecting."
