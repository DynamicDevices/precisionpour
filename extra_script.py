Import("env")

# Copy lv_conf.h to LVGL library directory
import os
import shutil

lvgl_dir = env["PROJECT_LIBDEPS_DIR"] + "/esp32dev/lvgl"
lv_conf_src = env["PROJECT_DIR"] + "/include/lv_conf.h"
lv_conf_dst = lvgl_dir + "/lv_conf.h"

if os.path.exists(lvgl_dir) and os.path.exists(lv_conf_src):
    shutil.copy2(lv_conf_src, lv_conf_dst)
    print(f"Copied lv_conf.h to {lv_conf_dst}")

# Copy User_Setup.h to TFT_eSPI library directory
tft_dir = env["PROJECT_LIBDEPS_DIR"] + "/esp32dev/TFT_eSPI"
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
