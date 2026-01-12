/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Unit tests for MQTT JSON parsing and validation
 */

#include <unity.h>
#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * Validate MQTT "paid" command JSON payload
 * Returns true if valid, false otherwise
 */
bool validate_paid_command(const char* json, char* unique_id_out, float* cost_per_ml_out, 
                          int* max_ml_out, char* currency_out, size_t currency_len) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        return false;
    }
    
    const char* unique_id = doc["id"] | "";
    float cost_per_ml = doc["cost_per_ml"] | 0.0;
    int max_ml = doc["max_ml"] | 0;
    const char* currency = doc["currency"] | "";
    
    // Validate fields
    if (strlen(unique_id) == 0 || strlen(unique_id) > 128) {
        return false;
    }
    if (cost_per_ml <= 0.0 || cost_per_ml > 1000.0) {
        return false;
    }
    if (max_ml <= 0 || max_ml > 100000) {
        return false;
    }
    if (strlen(currency) > 0 && strcmp(currency, "GBP") != 0 && strcmp(currency, "USD") != 0) {
        return false;
    }
    
    // Copy to output parameters
    if (unique_id_out) {
        strncpy(unique_id_out, unique_id, 128);
        unique_id_out[127] = '\0';
    }
    if (cost_per_ml_out) {
        *cost_per_ml_out = cost_per_ml;
    }
    if (max_ml_out) {
        *max_ml_out = max_ml;
    }
    if (currency_out && currency_len > 0) {
        strncpy(currency_out, currency, currency_len - 1);
        currency_out[currency_len - 1] = '\0';
    }
    
    return true;
}

void setUp(void) {
    // Set up test fixtures
}

void tearDown(void) {
    // Clean up after tests
}

/**
 * Test valid JSON payload
 */
void test_valid_json_payload(void) {
    const char* json = "{\"id\":\"test123\",\"cost_per_ml\":0.005,\"max_ml\":500,\"currency\":\"GBP\"}";
    char unique_id[128];
    float cost_per_ml;
    int max_ml;
    char currency[16];
    
    bool valid = validate_paid_command(json, unique_id, &cost_per_ml, &max_ml, currency, sizeof(currency));
    
    TEST_ASSERT_TRUE(valid);
    TEST_ASSERT_EQUAL_STRING("test123", unique_id);
    TEST_ASSERT_FLOAT_WITHIN(0.0001, 0.005, cost_per_ml);
    TEST_ASSERT_EQUAL(500, max_ml);
    TEST_ASSERT_EQUAL_STRING("GBP", currency);
}

/**
 * Test valid JSON without currency (optional field)
 */
void test_valid_json_no_currency(void) {
    const char* json = "{\"id\":\"test456\",\"cost_per_ml\":0.01,\"max_ml\":1000}";
    char unique_id[128];
    float cost_per_ml;
    int max_ml;
    char currency[16];
    
    bool valid = validate_paid_command(json, unique_id, &cost_per_ml, &max_ml, currency, sizeof(currency));
    
    TEST_ASSERT_TRUE(valid);
    TEST_ASSERT_EQUAL_STRING("test456", unique_id);
    TEST_ASSERT_FLOAT_WITHIN(0.0001, 0.01, cost_per_ml);
    TEST_ASSERT_EQUAL(1000, max_ml);
    TEST_ASSERT_EQUAL_STRING("", currency);
}

/**
 * Test invalid JSON (malformed)
 */
void test_invalid_json_malformed(void) {
    const char* json = "{\"id\":\"test\",\"cost_per_ml\":0.005";  // Missing closing brace
    bool valid = validate_paid_command(json, NULL, NULL, NULL, NULL, 0);
    TEST_ASSERT_FALSE(valid);
}

/**
 * Test invalid JSON (missing required field)
 */
void test_invalid_json_missing_field(void) {
    const char* json = "{\"id\":\"test\",\"cost_per_ml\":0.005}";  // Missing max_ml
    bool valid = validate_paid_command(json, NULL, NULL, NULL, NULL, 0);
    TEST_ASSERT_FALSE(valid);
}

/**
 * Test invalid JSON (invalid cost_per_ml - negative)
 */
void test_invalid_json_negative_cost(void) {
    const char* json = "{\"id\":\"test\",\"cost_per_ml\":-0.005,\"max_ml\":500}";
    bool valid = validate_paid_command(json, NULL, NULL, NULL, NULL, 0);
    TEST_ASSERT_FALSE(valid);
}

/**
 * Test invalid JSON (invalid max_ml - zero)
 */
void test_invalid_json_zero_max_ml(void) {
    const char* json = "{\"id\":\"test\",\"cost_per_ml\":0.005,\"max_ml\":0}";
    bool valid = validate_paid_command(json, NULL, NULL, NULL, NULL, 0);
    TEST_ASSERT_FALSE(valid);
}

/**
 * Test invalid JSON (invalid currency)
 */
void test_invalid_json_invalid_currency(void) {
    const char* json = "{\"id\":\"test\",\"cost_per_ml\":0.005,\"max_ml\":500,\"currency\":\"EUR\"}";
    bool valid = validate_paid_command(json, NULL, NULL, NULL, NULL, 0);
    TEST_ASSERT_FALSE(valid);
}

/**
 * Test valid JSON with USD currency
 */
void test_valid_json_usd_currency(void) {
    const char* json = "{\"id\":\"test789\",\"cost_per_ml\":0.01,\"max_ml\":750,\"currency\":\"USD\"}";
    char unique_id[128];
    float cost_per_ml;
    int max_ml;
    char currency[16];
    
    bool valid = validate_paid_command(json, unique_id, &cost_per_ml, &max_ml, currency, sizeof(currency));
    
    TEST_ASSERT_TRUE(valid);
    TEST_ASSERT_EQUAL_STRING("USD", currency);
}

void setup() {
    // Wait for serial monitor to connect (for native testing)
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_valid_json_payload);
    RUN_TEST(test_valid_json_no_currency);
    RUN_TEST(test_invalid_json_malformed);
    RUN_TEST(test_invalid_json_missing_field);
    RUN_TEST(test_invalid_json_negative_cost);
    RUN_TEST(test_invalid_json_zero_max_ml);
    RUN_TEST(test_invalid_json_invalid_currency);
    RUN_TEST(test_valid_json_usd_currency);
    
    UNITY_END();
}

void loop() {
    // Empty - tests run once in setup()
}
