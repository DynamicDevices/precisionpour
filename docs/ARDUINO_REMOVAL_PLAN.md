# Arduino Compatibility Layer Removal Plan

This document outlines a risk-prioritized plan to remove Arduino compatibility layer dependencies from the PrecisionPour codebase.

## Overview

The codebase currently uses two compatibility layers:
1. **`include/system/esp_idf_compat.h`** - Provides Arduino-like APIs (millis, delay, pinMode, etc.)
2. **`include/system/esp_system_compat.h`** - Provides ESP.* functions (getChipModel, etc.)

## Risk Assessment Criteria

- **Low Risk**: Deprecated files, unused functions, isolated components
- **Medium Risk**: Well-isolated components with moderate dependencies
- **High Risk**: Core files used throughout the system, critical functionality

## Removal Plan

### Phase 1: Low Risk - Deprecated Files (Estimated: 1-2 hours)

**Files to Remove:**
- `src/production_mode_ui.cpp` - Deprecated, replaced by component-based UI
- `src/pouring_mode_ui.cpp` - Deprecated, replaced by component-based UI
- `src/test_mode_ui.cpp` - Deprecated, replaced by component-based UI
- `src/ui.cpp` - Deprecated, replaced by component-based UI

**Dependencies:**
- Uses: `delay()`, `millis()`
- Impact: None (files are deprecated and not used)

**Action:**
1. Verify these files are not referenced in `CMakeLists.txt`
2. Delete the files
3. Remove any header files if they exist
4. Test build

**Success Criteria:**
- Build succeeds
- No references to deleted files

---

### Phase 2: Low Risk - Unused Functions (Estimated: 30 minutes)

**Functions to Remove:**
- `Serial` class - Not used anywhere (no `Serial.` calls found)
- Unused compatibility macros/defines

**Dependencies:**
- None (unused)

**Action:**
1. Remove `SerialClass` from `esp_idf_compat.h`
2. Remove `extern SerialClass Serial;`
3. Test build

**Success Criteria:**
- Build succeeds
- No compilation errors

---

### Phase 3: Medium Risk - WiFi Improv (Estimated: 1 hour)

**File:** `src/wifi/wifi_improv.cpp`

**Dependencies:**
- Uses: `ESP.restart()`, `delay()`, `millis()`
- Impact: WiFi provisioning via BLE

**Replacements:**
- `ESP.restart()` → `esp_restart()`
- `delay()` → `vTaskDelay(pdMS_TO_TICKS(ms))`
- `millis()` → `esp_timer_get_time() / 1000ULL`

**Action:**
1. Replace all Arduino compatibility calls with ESP-IDF equivalents
2. Remove `#include "system/esp_idf_compat.h"` and `#include "system/esp_system_compat.h"`
3. Add direct ESP-IDF includes as needed
4. Test WiFi provisioning functionality

**Success Criteria:**
- Build succeeds
- WiFi provisioning via BLE works correctly
- Device restarts properly after provisioning

---

### Phase 4: Medium Risk - MQTT Components (Estimated: 2 hours)

**Files:**
- `src/mqtt/mqtt_connection.cpp`
- `src/mqtt/mqtt_messages.cpp`

**Dependencies:**
- Uses: `millis()` for timing
- Impact: MQTT connection management and activity tracking

**Replacements:**
- `millis()` → `esp_timer_get_time() / 1000ULL`
- Handle overflow: `millis()` returns `unsigned long` (32-bit), `esp_timer_get_time()` returns `uint64_t`

**Action:**
1. Replace `millis()` calls with `esp_timer_get_time() / 1000ULL`
2. Update variable types if needed (`unsigned long` → `uint64_t` for overflow safety)
3. Remove `#include "system/esp_idf_compat.h"`
4. Test MQTT connection and message handling

**Success Criteria:**
- Build succeeds
- MQTT connects and reconnects correctly
- Activity tracking works (data icon flashes)

---

### Phase 5: Medium Risk - UI Icon Components (Estimated: 1-2 hours)

**Files:**
- `src/ui/ui_wifi_icon.cpp`
- `src/ui/ui_data_icon.cpp`

**Dependencies:**
- Uses: `millis()` for animation timing
- Impact: WiFi/data icon flashing animations

**Replacements:**
- `millis()` → `esp_timer_get_time() / 1000ULL`

**Action:**
1. Replace `millis()` calls
2. Remove `#include "system/esp_idf_compat.h"`
3. Test icon animations

**Success Criteria:**
- Build succeeds
- WiFi icon flashes when connecting
- Data icon flashes on MQTT activity

---

### Phase 6: Medium Risk - Screen Components (Estimated: 2-3 hours)

**Files:**
- `src/ui/screen_manager.cpp`
- `src/ui/splashscreen.cpp`
- `src/ui/finished_screen.cpp`
- `src/ui/pouring_screen.cpp`
- `src/ui/qr_code_screen.cpp`
- `src/ui/base_screen.cpp`

**Dependencies:**
- Uses: `delay()`, `millis()` for timing and delays
- Impact: Screen transitions, timeouts, animations

**Replacements:**
- `delay()` → `vTaskDelay(pdMS_TO_TICKS(ms))`
- `millis()` → `esp_timer_get_time() / 1000ULL`

**Action:**
1. Replace all `delay()` and `millis()` calls
2. Remove `#include "system/esp_idf_compat.h"` from each file
3. Test all screen transitions and timeouts

**Success Criteria:**
- Build succeeds
- All screen transitions work correctly
- Finished screen timeout works
- Debug tap navigation works

---

### Phase 7: High Risk - Display Components (Estimated: 3-4 hours)

**Files:**
- `src/display/lvgl_display.cpp`
- `src/display/lvgl_touch.cpp`

**Dependencies:**
- `lvgl_display.cpp`: Uses `delay()` for display initialization timing
- `lvgl_touch.cpp`: Uses `millis()`, `pinMode()`, `digitalWrite()`, `digitalRead()`, `delayMicroseconds()`, `attachInterrupt()`
- Impact: **CRITICAL** - Display and touch functionality

**Replacements:**
- `delay()` → `vTaskDelay(pdMS_TO_TICKS(ms))`
- `delayMicroseconds()` → `esp_rom_delay_us(us)` (already in `esp_idf_helpers.h`)
- `millis()` → `esp_timer_get_time() / 1000ULL`
- `pinMode()` → `gpio_config()` (native ESP-IDF)
- `digitalWrite()` → `gpio_set_level()`
- `digitalRead()` → `gpio_get_level()`
- `attachInterrupt()` → `gpio_config()` + `gpio_isr_handler_add()`

**Action:**
1. **lvgl_display.cpp:**
   - Replace `delay()` calls
   - Remove `#include "system/esp_idf_compat.h"`
   - Test display initialization

2. **lvgl_touch.cpp:**
   - Replace all GPIO and timing functions
   - Use native ESP-IDF GPIO APIs
   - Update interrupt handling to use native ESP-IDF
   - Remove `#include "system/esp_idf_compat.h"`
   - Test touch functionality thoroughly

**Success Criteria:**
- Build succeeds
- Display initializes correctly
- Touch input works reliably
- No touch event loss or false positives

---

### Phase 8: High Risk - Flow Meter (Estimated: 2-3 hours)

**File:** `src/flow/flow_meter.cpp`

**Dependencies:**
- Uses: `millis()`, `pinMode()`, `attachInterrupt()`
- Impact: **CRITICAL** - Flow measurement for pouring

**Replacements:**
- `millis()` → `esp_timer_get_time() / 1000ULL`
- `pinMode()` → `gpio_config()`
- `attachInterrupt()` → `gpio_config()` + `gpio_isr_handler_add()`

**Action:**
1. Replace all compatibility calls
2. Update interrupt handler to use native ESP-IDF
3. Remove `#include "system/esp_idf_compat.h"`
4. Test flow meter accuracy and responsiveness

**Success Criteria:**
- Build succeeds
- Flow meter detects pulses correctly
- Volume calculations are accurate
- No missed pulses

---

### Phase 9: High Risk - WiFi Manager (Estimated: 3-4 hours)

**File:** `src/wifi/wifi_manager.cpp`

**Dependencies:**
- Uses: `millis()`, `delay()`, `ESP.restart()` (via esp_system_compat)
- Impact: **CRITICAL** - WiFi connectivity

**Replacements:**
- `millis()` → `esp_timer_get_time() / 1000ULL`
- `delay()` → `vTaskDelay(pdMS_TO_TICKS(ms))`
- `ESP.restart()` → `esp_restart()`

**Action:**
1. Replace all timing and restart calls
2. Remove `#include "system/esp_idf_compat.h"` and `#include "system/esp_system_compat.h"`
3. Add direct ESP-IDF includes
4. Test WiFi connection, reconnection, and NTP sync

**Success Criteria:**
- Build succeeds
- WiFi connects and reconnects correctly
- NTP time sync works
- WiFi credentials are saved/loaded correctly

---

### Phase 10: High Risk - Main Application (Estimated: 4-5 hours)

**File:** `src/main.cpp`

**Dependencies:**
- Uses: `ESP.getChipModel()`, `ESP.getChipRevision()`, `ESP.getCpuFreqMHz()`, `ESP.getFlashChipSize()`, `ESP.getFreeHeap()`, `ESP.restart()`, `millis()`, `delay()`, `pinMode()`, `digitalWrite()`
- Impact: **CRITICAL** - Application entry point and core functionality

**Replacements:**
- `ESP.getChipModel()` → `esp_chip_info_t` + `esp_chip_info()`
- `ESP.getChipRevision()` → `esp_chip_info_t.revision`
- `ESP.getCpuFreqMHz()` → `esp_clk_cpu_freq() / 1000000`
- `ESP.getFlashChipSize()` → `esp_flash_get_size()`
- `ESP.getFreeHeap()` → `esp_get_free_heap_size()`
- `ESP.restart()` → `esp_restart()`
- `millis()` → `esp_timer_get_time() / 1000ULL`
- `delay()` → `vTaskDelay(pdMS_TO_TICKS(ms))`
- `pinMode()` → `gpio_config()`
- `digitalWrite()` → `gpio_set_level()`

**Action:**
1. Replace all ESP.* calls with native ESP-IDF APIs
2. Replace all timing and GPIO calls
3. Remove `#include "system/esp_idf_compat.h"` and `#include "system/esp_system_compat.h"`
4. Add necessary ESP-IDF includes:
   - `esp_chip_info.h`
   - `esp_flash.h`
   - `esp_system.h`
5. Test full application startup and operation

**Success Criteria:**
- Build succeeds
- Application starts correctly
- All initialization sequences work
- Watchdog functionality works
- Error handling and recovery work

---

### Phase 11: Final Cleanup (Estimated: 1 hour)

**Files to Remove:**
- `include/system/esp_idf_compat.h`
- `include/system/esp_system_compat.h`
- `src/system/esp_system_compat.cpp`
- `src/system/esp_idf_gpio_isr.cpp` (if only used by compatibility layer)

**Action:**
1. Verify no remaining references to compatibility files
2. Delete compatibility files
3. Update `CMakeLists.txt` if needed
4. Final build and test

**Success Criteria:**
- Build succeeds
- All functionality works
- No compilation errors or warnings
- Codebase is fully native ESP-IDF

---

## Testing Strategy

After each phase:
1. **Build Test**: Ensure code compiles without errors
2. **Functionality Test**: Test the specific functionality affected
3. **Integration Test**: Test that the change doesn't break other components
4. **Regression Test**: Verify existing functionality still works

## Estimated Total Time

- **Low Risk Phases**: 2-3 hours
- **Medium Risk Phases**: 8-12 hours
- **High Risk Phases**: 12-16 hours
- **Total**: ~22-31 hours

## Notes

- Each phase should be committed separately for easy rollback
- Test thoroughly after each phase before proceeding
- Some phases can be done in parallel if they don't share dependencies
- Consider creating helper functions in `include/utils/esp_idf_helpers.h` for common replacements (e.g., `get_time_ms()` wrapper)

## Helper Functions to Create

Consider adding to `include/utils/esp_idf_helpers.h`:
- `static inline uint64_t get_time_ms(void)` - Wrapper for `esp_timer_get_time() / 1000ULL`
- GPIO helper functions if needed
