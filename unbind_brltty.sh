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
