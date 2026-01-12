/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Unit tests for flow meter calculations
 */

#include <unity.h>
#include <Arduino.h>

// Mock flow meter calculation functions for testing
// These test the core calculation logic without hardware dependencies

// Flow meter constants (same as in flow_meter.cpp)
#define PULSES_PER_LITER 450
#define PULSES_PER_LPM 7.5

/**
 * Calculate flow rate from pulse frequency
 * This is the core calculation logic from flow_meter.cpp
 */
float calculate_flow_rate_from_pulses(unsigned long pulses_per_second) {
    float frequency_hz = (float)pulses_per_second;
    return frequency_hz / PULSES_PER_LPM;
}

/**
 * Calculate volume from pulse count
 */
float calculate_volume_from_pulses(unsigned long total_pulses) {
    return (float)total_pulses / PULSES_PER_LITER;
}

void setUp(void) {
    // Set up test fixtures
}

void tearDown(void) {
    // Clean up after tests
}

/**
 * Test flow rate calculation for zero flow
 */
void test_flow_rate_zero(void) {
    float flow_rate = calculate_flow_rate_from_pulses(0);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, flow_rate);
}

/**
 * Test flow rate calculation for 1 L/min
 * 1 L/min = 7.5 Hz = 7.5 pulses per second
 */
void test_flow_rate_1_lpm(void) {
    float flow_rate = calculate_flow_rate_from_pulses(7);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 1.0, flow_rate);
}

/**
 * Test flow rate calculation for 10 L/min
 * 10 L/min = 75 Hz = 75 pulses per second
 */
void test_flow_rate_10_lpm(void) {
    float flow_rate = calculate_flow_rate_from_pulses(75);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 10.0, flow_rate);
}

/**
 * Test flow rate calculation for 30 L/min (maximum)
 * 30 L/min = 225 Hz = 225 pulses per second
 */
void test_flow_rate_30_lpm(void) {
    float flow_rate = calculate_flow_rate_from_pulses(225);
    TEST_ASSERT_FLOAT_WITHIN(0.5, 30.0, flow_rate);
}

/**
 * Test volume calculation from pulses
 * 450 pulses = 1 liter
 */
void test_volume_calculation(void) {
    float volume = calculate_volume_from_pulses(450);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, volume);
    
    volume = calculate_volume_from_pulses(900);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, volume);
    
    volume = calculate_volume_from_pulses(225);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.5, volume);
}

/**
 * Test flow rate edge cases
 */
void test_flow_rate_edge_cases(void) {
    // Very low flow rate
    float flow_rate = calculate_flow_rate_from_pulses(1);
    TEST_ASSERT_TRUE(flow_rate > 0.0);
    TEST_ASSERT_TRUE(flow_rate < 1.0);
    
    // High flow rate (above spec but should handle gracefully)
    flow_rate = calculate_flow_rate_from_pulses(300);
    TEST_ASSERT_TRUE(flow_rate > 30.0);
}

/**
 * Test volume calculation edge cases
 */
void test_volume_edge_cases(void) {
    // Zero pulses
    float volume = calculate_volume_from_pulses(0);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, volume);
    
    // Single pulse
    volume = calculate_volume_from_pulses(1);
    TEST_ASSERT_TRUE(volume > 0.0);
    TEST_ASSERT_TRUE(volume < 0.01);
}

void setup() {
    // Wait for serial monitor to connect (for native testing)
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_flow_rate_zero);
    RUN_TEST(test_flow_rate_1_lpm);
    RUN_TEST(test_flow_rate_10_lpm);
    RUN_TEST(test_flow_rate_30_lpm);
    RUN_TEST(test_volume_calculation);
    RUN_TEST(test_flow_rate_edge_cases);
    RUN_TEST(test_volume_edge_cases);
    
    UNITY_END();
}

void loop() {
    // Empty - tests run once in setup()
}
