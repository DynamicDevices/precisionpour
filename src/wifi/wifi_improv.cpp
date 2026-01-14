/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * WiFi Improv Provisioning Implementation (ESP-IDF Native)
 * 
 * Handles Improv WiFi BLE provisioning for credential setup using ESP-IDF native BLE APIs
 * Based on the Improv WiFi protocol specification: https://www.improv-wifi.com/
 */

#include "config.h"
#include "wifi/wifi_improv.h"
#include "wifi/wifi_credentials.h"
#include "wifi/wifi_manager.h"

// System/Standard library headers
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>
#include <string>
#define TAG "wifi_improv"

// Early logging function (before ESP-IDF logging is initialized)
static void early_log(const char* msg) {
    esp_rom_printf("[EARLY] %s\n", msg);
}

// Constructor to log when wifi_improv static initialization happens
__attribute__((constructor))
static void wifi_improv_static_init_logger(void) {
    early_log("wifi_improv.cpp static initialization starting...");
    early_log("wifi_improv.cpp static initialization complete");
}

// Improv state (declared outside #if to be accessible everywhere)
static bool improv_provisioning_active = false;

// ESP-IDF BLE headers
#if USE_IMPROV_WIFI
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_gatt_common_api.h"

// UUID structures - uninitialized, will be initialized at runtime
// This avoids static initialization that might trigger BLE component static init
static esp_bt_uuid_t improv_service_uuid;
static esp_bt_uuid_t improv_status_uuid;
static esp_bt_uuid_t improv_error_uuid;
static esp_bt_uuid_t improv_rpc_uuid;
static esp_bt_uuid_t improv_capabilities_uuid;
static bool uuids_initialized = false;

// Initialize UUID structures at runtime (called after BLE is initialized)
static void init_improv_uuids(void) {
    if (uuids_initialized) return;
    
    // Service UUID: 00467768-6221-5521-6363-6f6d6d616e64
    improv_service_uuid.len = ESP_UUID_LEN_128;
    memcpy(improv_service_uuid.uuid.uuid128, 
           (uint8_t[]){0x00, 0x46, 0x77, 0x68, 0x62, 0x21, 0x55, 0x21,
                       0x63, 0x63, 0x6f, 0x6d, 0x6d, 0x61, 0x6e, 0x64}, 16);
    
    // Status UUID: ...-01
    improv_status_uuid.len = ESP_UUID_LEN_128;
    memcpy(improv_status_uuid.uuid.uuid128,
           (uint8_t[]){0x00, 0x46, 0x77, 0x68, 0x62, 0x21, 0x55, 0x21,
                       0x63, 0x63, 0x6f, 0x6d, 0x6d, 0x61, 0x6e, 0x01}, 16);
    
    // Error UUID: ...-02
    improv_error_uuid.len = ESP_UUID_LEN_128;
    memcpy(improv_error_uuid.uuid.uuid128,
           (uint8_t[]){0x00, 0x46, 0x77, 0x68, 0x62, 0x21, 0x55, 0x21,
                       0x63, 0x63, 0x6f, 0x6d, 0x6d, 0x61, 0x6e, 0x02}, 16);
    
    // RPC UUID: ...-03
    improv_rpc_uuid.len = ESP_UUID_LEN_128;
    memcpy(improv_rpc_uuid.uuid.uuid128,
           (uint8_t[]){0x00, 0x46, 0x77, 0x68, 0x62, 0x21, 0x55, 0x21,
                       0x63, 0x63, 0x6f, 0x6d, 0x6d, 0x61, 0x6e, 0x03}, 16);
    
    // Capabilities UUID: ...-04
    improv_capabilities_uuid.len = ESP_UUID_LEN_128;
    memcpy(improv_capabilities_uuid.uuid.uuid128,
           (uint8_t[]){0x00, 0x46, 0x77, 0x68, 0x62, 0x21, 0x55, 0x21,
                       0x63, 0x63, 0x6f, 0x6d, 0x6d, 0x61, 0x6e, 0x04}, 16);
    
    uuids_initialized = true;
}

// Improv protocol constants (from https://www.improv-wifi.com/)
#define IMPROV_STATUS_STOPPED          0x00
#define IMPROV_STATUS_AWAITING_AUTH    0x01
#define IMPROV_STATUS_AUTHORIZED       0x02
#define IMPROV_STATUS_PROVISIONING     0x03
#define IMPROV_STATUS_PROVISIONED      0x04

#define IMPROV_ERROR_NONE              0x00
#define IMPROV_ERROR_INVALID_RPC       0x01
#define IMPROV_ERROR_UNKNOWN_RPC       0x02
#define IMPROV_ERROR_UNABLE_TO_CONNECT 0x03
#define IMPROV_ERROR_NOT_AUTHORIZED    0x04

#define IMPROV_RPC_VERSION             0x01
#define IMPROV_RPC_GET_WIFI_NETWORKS   0x02
#define IMPROV_RPC_SET_WIFI_CREDENTIALS 0x03
#define IMPROV_RPC_GET_WIFI_STATUS     0x04

// Forward declaration - callback to connect to WiFi (provided by wifi_manager)
// Note: Temporarily using String for compatibility, will be updated in future refactoring
extern bool wifi_manager_connect(const String& ssid, const String& password);

// Initialize to 0xFF (ESP_GATT_IF_NONE) - avoid using constant during static init
static uint16_t improv_gatts_if = 0xFF;  // ESP_GATT_IF_NONE
static uint16_t improv_conn_id = 0;
static uint16_t improv_service_handle = 0;
static uint16_t improv_status_handle = 0;
static uint16_t improv_error_handle = 0;
static uint16_t improv_rpc_handle = 0;
static uint16_t improv_capabilities_handle = 0;

// Improv state
static uint8_t improv_status = IMPROV_STATUS_STOPPED;
static uint8_t improv_error = IMPROV_ERROR_NONE;

// GATT event handler
static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param);

// GAP event handler
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

// Helper function to convert C string to String (temporary compatibility)
static String cstr_to_string(const char* str) {
    if (str == nullptr) {
        return String("");
    }
    return String(str);
}

// Update status characteristic
static void update_status_characteristic() {
    if (improv_gatts_if != 0xFF && improv_status_handle != 0) {  // 0xFF = ESP_GATT_IF_NONE
        esp_ble_gatts_set_attr_value(improv_status_handle, 1, &improv_status);
    }
}

// Update error characteristic
static void update_error_characteristic() {
    if (improv_gatts_if != 0xFF && improv_error_handle != 0) {  // 0xFF = ESP_GATT_IF_NONE
        esp_ble_gatts_set_attr_value(improv_error_handle, 1, &improv_error);
    }
}

// Handle Improv RPC commands
static void handle_improv_rpc(const uint8_t* data, uint16_t len) {
    if (len < 1) {
        improv_error = IMPROV_ERROR_INVALID_RPC;
        update_error_characteristic();
        return;
    }
    
    uint8_t cmd = data[0];
    
    switch (cmd) {
        case IMPROV_RPC_VERSION: {
            // Return version: 1
            uint8_t response[] = {IMPROV_RPC_VERSION, 0x01};
            esp_ble_gatts_send_indicate(improv_gatts_if, improv_conn_id, improv_rpc_handle,
                                       sizeof(response), response, false);
            break;
        }
        
        case IMPROV_RPC_GET_WIFI_NETWORKS: {
            // Return empty list (we don't scan for networks)
            uint8_t response[] = {IMPROV_RPC_GET_WIFI_NETWORKS, 0x00};
            esp_ble_gatts_send_indicate(improv_gatts_if, improv_conn_id, improv_rpc_handle,
                                       sizeof(response), response, false);
            break;
        }
        
        case IMPROV_RPC_SET_WIFI_CREDENTIALS: {
            if (len < 3) {
                improv_error = IMPROV_ERROR_INVALID_RPC;
                update_error_characteristic();
                break;
            }
            
            // Parse SSID and password from RPC data
            // Format: [cmd, ssid_len, ssid_bytes..., password_len, password_bytes...]
            uint8_t ssid_len = data[1];
            if (ssid_len == 0 || ssid_len > 32 || len < (2 + ssid_len + 1)) {
                improv_error = IMPROV_ERROR_INVALID_RPC;
                update_error_characteristic();
                break;
            }
            
            uint8_t password_len = data[2 + ssid_len];
            if (password_len > 64 || len < (3 + ssid_len + password_len)) {
                improv_error = IMPROV_ERROR_INVALID_RPC;
                update_error_characteristic();
                break;
            }
            
            // Extract SSID and password
            char ssid[33] = {0};
            char password[65] = {0};
            memcpy(ssid, &data[2], ssid_len);
            memcpy(password, &data[3 + ssid_len], password_len);
            
            ESP_LOGI(TAG, "[Improv WiFi BLE] Received credentials for: %s", ssid);
            
            improv_status = IMPROV_STATUS_PROVISIONING;
            improv_error = IMPROV_ERROR_NONE;
            update_status_characteristic();
            update_error_characteristic();
            
            // Deinitialize BLE first before re-enabling WiFi
            esp_ble_gatts_stop_service(improv_service_handle);
            esp_ble_gatts_delete_service(improv_service_handle);
            esp_bluedroid_disable();
            esp_bluedroid_deinit();
            esp_bt_controller_disable();
            esp_bt_controller_deinit();
            vTaskDelay(pdMS_TO_TICKS(200));
            
            // Re-enable WiFi
            esp_wifi_start();
            vTaskDelay(pdMS_TO_TICKS(100));
            
            // Try to connect with new credentials
            if (wifi_manager_connect(cstr_to_string(ssid), cstr_to_string(password))) {
                // Save credentials for future use
                wifi_credentials_save(cstr_to_string(ssid), cstr_to_string(password));
                improv_provisioning_active = false;
                ESP_LOGI(TAG, "[Improv WiFi BLE] Provisioning successful!");
                ESP_LOGI(TAG, "[Improv WiFi BLE] Credentials saved, restarting device...");
                vTaskDelay(pdMS_TO_TICKS(1000));
                esp_restart();
            } else {
                ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to connect with provided credentials");
                improv_error = IMPROV_ERROR_UNABLE_TO_CONNECT;
                // Note: BLE is already deinitialized, so we can't send error response
                // Restart BLE provisioning
                vTaskDelay(pdMS_TO_TICKS(1000));
                wifi_improv_start_provisioning();
            }
            break;
        }
        
        case IMPROV_RPC_GET_WIFI_STATUS: {
            // Return current WiFi status
            bool connected = wifi_manager_is_connected();
            uint8_t response[] = {IMPROV_RPC_GET_WIFI_STATUS, static_cast<uint8_t>(connected ? 0x01 : 0x00)};
            esp_ble_gatts_send_indicate(improv_gatts_if, improv_conn_id, improv_rpc_handle,
                                       sizeof(response), response, false);
            break;
        }
        
        default:
            improv_error = IMPROV_ERROR_UNKNOWN_RPC;
            update_error_characteristic();
            break;
    }
}

// GATT event handler implementation
static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            improv_gatts_if = gatts_if;
            ESP_LOGI(TAG, "[Improv WiFi BLE] GATT server registered, creating service...");
            
            // Create Improv service
            esp_gatt_srvc_id_t service_id = {
                .id = {
                    .uuid = improv_service_uuid,
                    .inst_id = 0
                },
                .is_primary = true
            };
            esp_ble_gatts_create_service(gatts_if, &service_id, 20);
            break;
        }
        
        case ESP_GATTS_CREATE_EVT: {
            if (param->create.status != ESP_GATT_OK) {
                ESP_LOGE(TAG, "[Improv WiFi BLE] Service creation failed: status=%d", param->create.status);
                improv_error = IMPROV_ERROR_INVALID_RPC;
                update_error_characteristic();
                improv_provisioning_active = false;
                break;
            }
            improv_service_handle = param->create.service_handle;
            if (improv_service_handle == 0) {
                ESP_LOGE(TAG, "[Improv WiFi BLE] Service creation returned invalid handle");
                improv_provisioning_active = false;
                break;
            }
            ESP_LOGI(TAG, "[Improv WiFi BLE] Service created, adding characteristics...");
            
            // Create Status characteristic (read, notify)
            esp_err_t ret = esp_ble_gatts_add_char(improv_service_handle, &improv_status_uuid,
                                   ESP_GATT_PERM_READ,
                                   ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                                   NULL, NULL);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to add Status characteristic: %s", esp_err_to_name(ret));
                improv_provisioning_active = false;
                break;
            }
            
            // Create Error characteristic (read, notify)
            ret = esp_ble_gatts_add_char(improv_service_handle, &improv_error_uuid,
                                   ESP_GATT_PERM_READ,
                                   ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                                   NULL, NULL);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to add Error characteristic: %s", esp_err_to_name(ret));
                improv_provisioning_active = false;
                break;
            }
            
            // Create RPC characteristic (write, notify)
            ret = esp_ble_gatts_add_char(improv_service_handle, &improv_rpc_uuid,
                                   ESP_GATT_PERM_WRITE,
                                   ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                                   NULL, NULL);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to add RPC characteristic: %s", esp_err_to_name(ret));
                improv_provisioning_active = false;
                break;
            }
            
            // Create Capabilities characteristic (read)
            // BLE capability = 0x01
            static uint8_t capabilities_data = 0x01;
            esp_attr_value_t capabilities_val;
            memset(&capabilities_val, 0, sizeof(esp_attr_value_t));
            capabilities_val.attr_max_len = 1;
            capabilities_val.attr_len = 1;
            capabilities_val.attr_value = &capabilities_data;
            ret = esp_ble_gatts_add_char(improv_service_handle, &improv_capabilities_uuid,
                                   ESP_GATT_PERM_READ,
                                   ESP_GATT_CHAR_PROP_BIT_READ,
                                   &capabilities_val, NULL);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to add Capabilities characteristic: %s", esp_err_to_name(ret));
                improv_provisioning_active = false;
                break;
            }

            // Start the service once characteristics are enqueued
            ret = esp_ble_gatts_start_service(improv_service_handle);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to start service: %s", esp_err_to_name(ret));
                improv_provisioning_active = false;
                break;
            }
            break;
        }
        
        case ESP_GATTS_ADD_CHAR_EVT: {
            if (param->add_char.status != ESP_GATT_OK) {
                ESP_LOGE(TAG, "[Improv WiFi BLE] Add characteristic failed: status=%d", param->add_char.status);
                improv_provisioning_active = false;
                break;
            }
            // Store characteristic handles
            if (memcmp(param->add_char.char_uuid.uuid.uuid128, improv_status_uuid.uuid.uuid128, 16) == 0) {
                improv_status_handle = param->add_char.attr_handle;
                ESP_LOGI(TAG, "[Improv WiFi BLE] Status characteristic added");
            } else if (memcmp(param->add_char.char_uuid.uuid.uuid128, improv_error_uuid.uuid.uuid128, 16) == 0) {
                improv_error_handle = param->add_char.attr_handle;
                ESP_LOGI(TAG, "[Improv WiFi BLE] Error characteristic added");
            } else if (memcmp(param->add_char.char_uuid.uuid.uuid128, improv_rpc_uuid.uuid.uuid128, 16) == 0) {
                improv_rpc_handle = param->add_char.attr_handle;
                ESP_LOGI(TAG, "[Improv WiFi BLE] RPC characteristic added");
            } else if (memcmp(param->add_char.char_uuid.uuid.uuid128, improv_capabilities_uuid.uuid.uuid128, 16) == 0) {
                improv_capabilities_handle = param->add_char.attr_handle;
                ESP_LOGI(TAG, "[Improv WiFi BLE] Capabilities characteristic added");
            }
            break;
        }
        
        case ESP_GATTS_START_EVT: {
            ESP_LOGI(TAG, "[Improv WiFi BLE] Service started, setting up advertising...");
            
            // Set up BLE advertising with Improv service UUID
            esp_ble_adv_data_t adv_data;
            memset(&adv_data, 0, sizeof(esp_ble_adv_data_t));
            adv_data.set_scan_rsp = false;
            adv_data.include_name = true;
            adv_data.include_txpower = false;
            adv_data.min_interval = 0x0006; // 7.5ms
            adv_data.max_interval = 0x0010; // 20ms
            adv_data.appearance = 0x00;
            adv_data.manufacturer_len = 0;
            adv_data.p_manufacturer_data = NULL;
            adv_data.service_data_len = 0;
            adv_data.p_service_data = NULL;
            adv_data.service_uuid_len = sizeof(improv_service_uuid.uuid.uuid128);
            adv_data.p_service_uuid = (uint8_t*)improv_service_uuid.uuid.uuid128;
            adv_data.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
            
            esp_ble_gap_config_adv_data(&adv_data);
            break;
        }
        
        case ESP_GATTS_CONNECT_EVT: {
            improv_conn_id = param->connect.conn_id;
            improv_status = IMPROV_STATUS_AWAITING_AUTH;
            update_status_characteristic();
            ESP_LOGI(TAG, "[Improv WiFi BLE] Client connected");
            break;
        }
        
        case ESP_GATTS_DISCONNECT_EVT: {
            improv_status = IMPROV_STATUS_STOPPED;
            improv_conn_id = 0;
            update_status_characteristic();
            ESP_LOGI(TAG, "[Improv WiFi BLE] Client disconnected");
            break;
        }
        
        case ESP_GATTS_WRITE_EVT: {
            if (param->write.handle == improv_rpc_handle) {
                handle_improv_rpc(param->write.value, param->write.len);
            }
            break;
        }
        
        default:
            break;
    }
}

// Advertisement parameters - initialized at runtime to avoid static init issues
// (BLE constants may trigger BLE component static initializers if used during static init)
static esp_ble_adv_params_t adv_params;

// Initialize advertisement parameters (called after BLE is initialized)
static void init_adv_params(void) {
    memset(&adv_params, 0, sizeof(esp_ble_adv_params_t));
    adv_params.adv_int_min = 0x20;
    adv_params.adv_int_max = 0x40;
    adv_params.adv_type = ADV_TYPE_IND;
    adv_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
    memset(adv_params.peer_addr, 0, sizeof(adv_params.peer_addr));
    adv_params.peer_addr_type = BLE_ADDR_TYPE_PUBLIC;
    adv_params.channel_map = ADV_CHNL_ALL;
    adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
}

// GAP event handler implementation
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "[Improv WiFi BLE] Advertisement data set, starting advertising...");
            // Ensure adv_params is initialized before use
            if (adv_params.adv_type == 0) {
                init_adv_params();
            }
            esp_ble_gap_start_advertising(&adv_params);
            break;
        
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to start advertising: %d", param->adv_start_cmpl.status);
            } else {
                ESP_LOGI(TAG, "[Improv WiFi BLE] Advertising started successfully");
            }
            break;
        
        default:
            break;
    }
}

#endif // USE_IMPROV_WIFI

void wifi_improv_start_provisioning() {
    #if USE_IMPROV_WIFI
    if (improv_provisioning_active) {
        return;  // Already provisioning
    }
    
    // Include BLE headers HERE (at function scope) to avoid static init issues
    // This ensures BLE headers are only included when we actually need BLE
    #include "esp_bt.h"
    #include "esp_bt_main.h"
    #include "esp_gap_ble_api.h"
    #include "esp_gatts_api.h"
    #include "esp_bt_defs.h"
    #include "esp_gatt_common_api.h"
    
    ESP_LOGI(TAG, "[Improv WiFi BLE] Starting BLE provisioning (ESP-IDF native)...");
    
    // Initialize UUID structures at runtime (avoid static init issues)
    init_improv_uuids();
    
    // Disable WiFi before initializing BLE to avoid coexistence conflicts
    ESP_LOGI(TAG, "[Improv WiFi BLE] Disabling WiFi for BLE...");
    esp_wifi_stop();
    esp_wifi_deinit();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Release Classic Bluetooth memory (we only use BLE)
    // This may help avoid static initialization issues
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    
    // Initialize BLE controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to initialize BLE controller: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to enable BLE controller: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to initialize bluedroid: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to enable bluedroid: %s", esp_err_to_name(ret));
        return;
    }
    
    // Set a friendly BLE device name so the user can easily find it in the Improv client.
    // Format: precisionpour-<last3mac> (e.g. precisionpour-2D9200)
    uint8_t mac[6] = {0};
    char ble_name[32] = {0};
    if (esp_efuse_mac_get_default(mac) == ESP_OK) {
        snprintf(ble_name, sizeof(ble_name), "precisionpour-%02X%02X%02X", mac[3], mac[4], mac[5]);
    } else {
        snprintf(ble_name, sizeof(ble_name), "precisionpour");
    }
    esp_ble_gap_set_device_name(ble_name);
    ESP_LOGI(TAG, "[Improv WiFi BLE] BLE device name: %s", ble_name);

    // Register GATT and GAP callbacks
    ret = esp_ble_gatts_register_callback(gatts_profile_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to register GATT callback: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to register GAP callback: %s", esp_err_to_name(ret));
        return;
    }
    
    // Register GATT application
    ret = esp_ble_gatts_app_register(0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to register GATT app: %s", esp_err_to_name(ret));
        return;
    }
    
    // Initialize advertisement parameters now that BLE is ready
    // (This avoids static initialization issues with BLE constants)
    init_adv_params();
    
    improv_provisioning_active = true;
    improv_status = IMPROV_STATUS_STOPPED;
    improv_error = IMPROV_ERROR_NONE;
    
    ESP_LOGI(TAG, "[Improv WiFi BLE] BLE provisioning initialized");
    ESP_LOGI(TAG, "[Improv WiFi BLE] Connect with Improv WiFi mobile app or web client");
    #else
    ESP_LOGI(TAG, "[Improv WiFi] Improv WiFi is disabled in config.h");
    #endif
}

bool wifi_improv_is_provisioning() {
    return improv_provisioning_active;
}

void wifi_improv_loop() {
    #if USE_IMPROV_WIFI
    // Handle Improv WiFi BLE provisioning if active
    if (improv_provisioning_active) {
        // Check for timeout
        static uint64_t provisioning_start = 0;
        if (provisioning_start == 0) {
            provisioning_start = esp_timer_get_time() / 1000ULL;
        }
        
        uint64_t now = esp_timer_get_time() / 1000ULL;
        if (now - provisioning_start > IMPROV_WIFI_TIMEOUT_MS) {
            ESP_LOGW(TAG, "[Improv WiFi BLE] Provisioning timeout - restarting device");
            improv_provisioning_active = false;
            provisioning_start = 0;
            
            // Deinitialize BLE
            esp_bluedroid_disable();
            esp_bluedroid_deinit();
            esp_bt_controller_disable();
            esp_bt_controller_deinit();
            vTaskDelay(pdMS_TO_TICKS(200));
            
            ESP_LOGI(TAG, "[Improv WiFi BLE] Restarting device after timeout...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        }
    }
    #endif
}