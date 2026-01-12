#!/bin/bash
#
# Copyright (c) 2026 Dynamic Devices Ltd
# All rights reserved.
#
# Script to unbind CH341 device from brltty
# This is called automatically by udev rules when a CH341 device is connected
#

set -euo pipefail

# Kill any running brltty processes
pkill -9 brltty 2>/dev/null || true

# Find and unbind CH341 devices from brltty
for dev in /sys/bus/usb/devices/*/idVendor; do
    if [ ! -f "$dev" ]; then
        continue
    fi

    vendor=$(cat "$dev" 2>/dev/null || echo "")
    if [ -z "$vendor" ]; then
        continue
    fi

    product_file=$(dirname "$dev")/idProduct
    if [ ! -f "$product_file" ]; then
        continue
    fi

    product=$(cat "$product_file" 2>/dev/null || echo "")
    if [ "$vendor" != "1a86" ] || [ "$product" != "7523" ]; then
        continue
    fi

    device_path=$(dirname "$dev")
    if [ ! -d "$device_path/driver" ]; then
        continue
    fi

    driver=$(readlink "$device_path/driver" 2>/dev/null | xargs basename || echo "")
    if [ "$driver" = "brltty" ]; then
        device_id=$(basename "$device_path")
        echo "$device_id" | sudo tee "$device_path/driver/unbind" > /dev/null 2>&1 || true
    fi
done
