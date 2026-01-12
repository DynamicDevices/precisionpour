Import("env")

# Copy lv_conf.h to LVGL library directory
import os
import shutil

# Get environment name from PIOENV (PlatformIO environment variable)
env_name = env.get("PIOENV", "esp32dev")  # Default to esp32dev if not set
libdeps_dir = env["PROJECT_LIBDEPS_DIR"]

# Build the full path: PROJECT_LIBDEPS_DIR/env_name/lvgl
lvgl_dir = os.path.join(libdeps_dir, env_name, "lvgl")
lv_conf_src = os.path.join(env["PROJECT_DIR"], "include", "lv_conf.h")
lv_conf_dst = os.path.join(lvgl_dir, "lv_conf.h")

if os.path.exists(lvgl_dir) and os.path.exists(lv_conf_src):
    shutil.copy2(lv_conf_src, lv_conf_dst)
    print(f"[extra_script] Copied lv_conf.h to {lv_conf_dst}")
else:
    if not os.path.exists(lvgl_dir):
        print(f"[extra_script] Warning: LVGL directory not found: {lvgl_dir}")
        # Try to wait a bit and retry (libraries might still be downloading)
        import time
        time.sleep(1)
        if os.path.exists(lvgl_dir) and os.path.exists(lv_conf_src):
            shutil.copy2(lv_conf_src, lv_conf_dst)
            print(f"[extra_script] Copied lv_conf.h to {lv_conf_dst} (retry)")
    if not os.path.exists(lv_conf_src):
        print(f"[extra_script] Warning: lv_conf.h source not found: {lv_conf_src}")

# Copy User_Setup.h to TFT_eSPI library directory (only for Arduino framework)
if env_name == "esp32dev":  # Only needed for Arduino framework
    tft_dir = libdeps_dir + "/TFT_eSPI"
    user_setup_src = env["PROJECT_DIR"] + "/lib/TFT_eSPI_Config/User_Setup.h"
    user_setup_dst = tft_dir + "/User_Setup.h"

    if os.path.exists(tft_dir) and os.path.exists(user_setup_src):
        shutil.copy2(user_setup_src, user_setup_dst)
        print(f"Copied User_Setup.h to {user_setup_dst}")
        
        # Also ensure driver defines are accessible by adding include path
        # The driver defines should be included by User_Setup_Select.h, but ensure they're available
        driver_defines = tft_dir + "/TFT_Drivers/ILI9341_Defines.h"
        if os.path.exists(driver_defines):
            print(f"Driver defines found at {driver_defines}")
