#!/bin/bash
#
# Copyright (c) 2026 Dynamic Devices Ltd
# All rights reserved.
#
# Script to completely prevent brltty from claiming CH341 USB-to-serial devices
# This fixes ESP32 disconnection issues caused by brltty interference
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
UNBIND_SCRIPT="${SCRIPT_DIR}/unbind_brltty.sh"

echo "=== Preventing brltty from claiming CH341 devices ==="
echo ""

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

# 4. Create unbind script
echo "4. Creating unbind script..."
sudo tee "${UNBIND_SCRIPT}" > /dev/null << 'EOFSCRIPT'
#!/bin/bash
# Script to unbind CH341 device from brltty
pkill -9 brltty 2>/dev/null || true
for dev in /sys/bus/usb/devices/*/idVendor; do
    if [ -f "$dev" ]; then
        vendor=$(cat "$dev" 2>/dev/null)
        product_file=$(dirname "$dev")/idProduct
        if [ -f "$product_file" ]; then
            product=$(cat "$product_file" 2>/dev/null)
            if [ "$vendor" = "1a86" ] && [ "$product" = "7523" ]; then
                device_path=$(dirname "$dev")
                if [ -d "$device_path/driver" ]; then
                    driver=$(readlink "$device_path/driver" 2>/dev/null | xargs basename)
                    if [ "$driver" = "brltty" ]; then
                        echo "$(basename $device_path)" | tee "$device_path/driver/unbind" 2>/dev/null || true
                    fi
                fi
            fi
        fi
    fi
done
EOFSCRIPT
sudo chmod +x "${UNBIND_SCRIPT}"

# 5. Create udev rules to prevent brltty from claiming CH341 devices
echo "5. Creating udev rules..."

# Rule 1: Prevent brltty from claiming
sudo tee /etc/udev/rules.d/99-prevent-brltty-ch341.rules > /dev/null << 'EOF'
# Prevent brltty from claiming CH341 USB-to-serial devices
ACTION=="add", SUBSYSTEM=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", ENV{BRLTTY_BRAILLE_DRIVER}=""
KERNEL=="ttyUSB*", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", ENV{BRLTTY_BRAILLE_DRIVER}="", MODE="0666", GROUP="dialout"
EOF

# Rule 2: Aggressively unbind from brltty
sudo tee /etc/udev/rules.d/98-unbind-brltty-ch341.rules > /dev/null << EOFRULE
ACTION=="add", SUBSYSTEM=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", RUN+="/bin/sh -c '${UNBIND_SCRIPT}'"
EOFRULE

# 6. Reload udev rules
echo "6. Reloading udev rules..."
sudo udevadm control --reload-rules
sudo udevadm trigger

echo ""
echo "=== Done ==="
echo "brltty has been prevented from claiming CH341 devices."
echo ""
echo "Now unplug and replug your ESP32 device."
echo "It should stay connected as /dev/ttyUSB* without disconnecting."
