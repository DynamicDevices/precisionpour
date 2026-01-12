cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Set project name
project(precisionpour)

# Required ESP-IDF components
set(EXTRA_COMPONENT_DIRS "components")

# Note: ArduinoJson should work with ESP-IDF as it's mostly header-only
# TFT_eSPI and PubSubClient will need to be replaced with ESP-IDF equivalents
