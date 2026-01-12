#!/usr/bin/env python3
#
# Copyright (c) 2026 Dynamic Devices Ltd
# All rights reserved.
#
# PlatformIO extra script to copy configuration files to library directories
# - Copies lv_conf.h to LVGL library directory
#

Import("env")

import os
import shutil
import time

def copy_lv_conf():
    """Copy lv_conf.h to LVGL library directory"""
    env_name = env.get("PIOENV", "esp32dev-idf")
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


# Main execution
if __name__ == "__main__":
    copy_lv_conf()
