/**
 * Pouring Mode UI
 * 
 * Screen displayed during active pouring, showing flow rate, volume, and cost
 */

#ifndef POURING_MODE_UI_H
#define POURING_MODE_UI_H

#include <lvgl.h>

void pouring_mode_init();
void pouring_mode_update();
void pouring_mode_reset();  // Reset volume and cost for new pour
void pouring_mode_set_cost_per_unit(float cost);  // Update cost per unit (e.g., from MQTT)
void pouring_mode_start_pour(const char* unique_id, float cost_per_ml, int max_ml, const char* currency);  // Start pouring with paid parameters
void pouring_mode_update_pour_params(const char* unique_id, float cost_per_ml, int max_ml, const char* currency);  // Update pour parameters
bool pouring_mode_is_max_reached();  // Check if max ml has been reached
void pouring_mode_set_screen_switch_callback(void (*callback)(void));  // Set callback to switch back to main screen

#endif // POURING_MODE_UI_H
