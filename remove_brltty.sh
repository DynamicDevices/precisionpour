#!/bin/bash
# Script to completely remove brltty

echo "=== Removing brltty completely ==="

# 1. Kill all brltty processes
echo "1. Killing brltty processes..."
sudo pkill -9 brltty 2>/dev/null || true
sudo killall -9 brltty 2>/dev/null || true

# 2. Stop and disable brltty service
echo "2. Stopping and disabling brltty service..."
sudo systemctl stop brltty 2>/dev/null || true
sudo systemctl disable brltty 2>/dev/null || true
sudo systemctl mask brltty 2>/dev/null || true

# 3. Remove brltty package
echo "3. Removing brltty package..."
sudo apt-get remove --purge -y brltty

# 4. Blacklist brltty module
echo "4. Blacklisting brltty module..."
echo "blacklist brltty" | sudo tee /etc/modprobe.d/blacklist-brltty.conf > /dev/null

# 5. Remove brltty module if loaded
echo "5. Removing brltty module..."
sudo modprobe -r brltty 2>/dev/null || echo "   brltty module not loaded"

# 6. Clean up any remaining brltty files
echo "6. Cleaning up brltty files..."
sudo rm -f /var/run/brltty.pid 2>/dev/null || true

echo ""
echo "=== Done ==="
echo "brltty has been completely removed."
echo ""
echo "Now unplug and replug your ESP32 device."
echo "It should stay connected without brltty interference."
