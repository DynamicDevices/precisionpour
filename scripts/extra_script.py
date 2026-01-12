#!/usr/bin/env python3
#
# Copyright (c) 2026 Dynamic Devices Ltd
# All rights reserved.
#
# PlatformIO extra script to copy configuration files to library directories
# - Copies lv_conf.h to LVGL library directory
# - Copies User_Setup.h to TFT_eSPI library directory (Arduino framework only)
#

Import("env")

import os
import shutil
import time

def copy_lv_conf():
    """Copy lv_conf.h to LVGL library directory"""
    env_name = env.get("PIOENV", "esp32dev")
    libdeps_dir = env["PROJECT_LIBDEPS_DIR"]
    project_dir = env["PROJECT_DIR"]

    # Build the full path: PROJECT_LIBDEPS_DIR/env_name/lvgl
    lvgl_dir = os.path.join(libdeps_dir, env_name, "lvgl")
    lv_conf_src = os.path.join(project_dir, "include", "lv_conf.h")
    lv_conf_dst = os.path.join(lvgl_dir, "lv_conf.h")

    if not os.path.exists(lv_conf_src):
        print(f"[extra_script] Warning: lv_conf.h source not found: {lv_conf_src}")
        return False

    if not os.path.exists(lvgl_dir):
        print(f"[extra_script] Warning: LVGL directory not found: {lvgl_dir}")
        # Try to wait a bit and retry (libraries might still be downloading)
        time.sleep(1)
        if not os.path.exists(lvgl_dir):
            print(f"[extra_script] Error: LVGL directory still not found after retry")
            return False

    try:
        shutil.copy2(lv_conf_src, lv_conf_dst)
        print(f"[extra_script] Copied lv_conf.h to {lv_conf_dst}")
        return True
    except Exception as e:
        print(f"[extra_script] Error copying lv_conf.h: {e}")
        return False


def copy_user_setup():
    """Copy User_Setup.h to TFT_eSPI library directory (Arduino framework only)"""
    env_name = env.get("PIOENV", "esp32dev")
    
    # Only needed for Arduino framework
    if env_name != "esp32dev":
        return True

    libdeps_dir = env["PROJECT_LIBDEPS_DIR"]
    project_dir = env["PROJECT_DIR"]

    tft_dir = os.path.join(libdeps_dir, "TFT_eSPI")
    user_setup_src = os.path.join(project_dir, "lib", "TFT_eSPI_Config", "User_Setup.h")
    user_setup_dst = os.path.join(tft_dir, "User_Setup.h")

    if not os.path.exists(user_setup_src):
        print(f"[extra_script] Warning: User_Setup.h source not found: {user_setup_src}")
        return False

    if not os.path.exists(tft_dir):
        print(f"[extra_script] Warning: TFT_eSPI directory not found: {tft_dir}")
        return False

    try:
        shutil.copy2(user_setup_src, user_setup_dst)
        print(f"[extra_script] Copied User_Setup.h to {user_setup_dst}")
        
        # Verify driver defines are accessible
        driver_defines = os.path.join(tft_dir, "TFT_Drivers", "ILI9341_Defines.h")
        if os.path.exists(driver_defines):
            print(f"[extra_script] Driver defines found at {driver_defines}")
        
        return True
    except Exception as e:
        print(f"[extra_script] Error copying User_Setup.h: {e}")
        return False


# Main execution
if __name__ == "__main__":
    copy_lv_conf()
    copy_user_setup()
