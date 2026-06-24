# Fetch and build Eclipse Paho MQTT C/C++ libraries when system packages are unavailable.
# Uses the paho.mqtt.cpp bundled paho.mqtt.c submodule so both libraries share one CMake project.

include(FetchContent)

set(PAHO_WITH_MQTT_C ON CACHE BOOL "" FORCE)
set(PAHO_WITH_SSL OFF CACHE BOOL "" FORCE)
set(PAHO_BUILD_STATIC OFF CACHE BOOL "" FORCE)
set(PAHO_BUILD_SHARED ON CACHE BOOL "" FORCE)
set(PAHO_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(PAHO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PAHO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(PAHO_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(PAHO_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(PAHO_ENABLE_CPACK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  paho_mqtt_cpp
  GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.cpp.git
  GIT_TAG v1.3.2
  GIT_SHALLOW TRUE
  GIT_SUBMODULES "src/externals/paho-mqtt-c"
)
FetchContent_MakeAvailable(paho_mqtt_cpp)

if(TARGET PahoMqttCpp::paho-mqttpp3)
  set(CVP_PAHO_MQTT_TARGET PahoMqttCpp::paho-mqttpp3)
elseif(TARGET paho-mqttpp3-static)
  set(CVP_PAHO_MQTT_TARGET paho-mqttpp3-static)
elseif(TARGET paho-mqttpp3)
  set(CVP_PAHO_MQTT_TARGET paho-mqttpp3)
else()
  message(FATAL_ERROR "Paho MQTT C++ target was not found after FetchContent.")
endif()
