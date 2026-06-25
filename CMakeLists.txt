cmake_minimum_required(VERSION 3.21)
project(ConnectedVehiclePlatform LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
include(FetchContent)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_compile_options(-Wall -Wextra -Wpedantic)

option(CVP_USE_SYSTEM_PAHO "Use system-installed Paho MQTT" OFF)
option(CVP_ENABLE_COMMONAPI_CODEGEN "Regenerate CommonAPI/SOMEIP sources" OFF)

find_package(Threads REQUIRED)

# -----------------------------
# PAHO MQTT
# -----------------------------
if(CVP_USE_SYSTEM_PAHO)
    find_package(PahoMqttCpp REQUIRED)
    set(CVP_PAHO_MQTT_TARGET PahoMqttCpp::paho-mqttpp3)
else()
    include(FetchPahoMqtt)
endif()

# -----------------------------
# CommonAPI Codegen
# -----------------------------
set(GEN_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/generated)
set(GEN_COMMONAPI ${GEN_ROOT}/v1)
set(GEN_PROXY ${GEN_ROOT}/proxy)
set(GEN_STUB ${GEN_ROOT}/stub)
set(GEN_SRC_GEN ${GEN_ROOT}/src-gen/v1)

if(CVP_ENABLE_COMMONAPI_CODEGEN)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    file(GLOB FIDL_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fidl/*.fidl)
    find_package(CommonAPI REQUIRED)

    if(NOT COMMONAPI_CORE_GENERATOR OR NOT COMMONAPI_SOMEIP_GENERATOR)
        message(FATAL_ERROR "CommonAPI generators missing.")
    endif()

    add_custom_command(
        OUTPUT ${GEN_ROOT}/.stamp
        COMMAND ${CMAKE_COMMAND} -E make_directory
            ${GEN_COMMONAPI} ${GEN_PROXY} ${GEN_STUB} ${GEN_SRC_GEN}
        COMMAND ${COMMONAPI_CORE_GENERATOR} -sk ${CMAKE_CURRENT_SOURCE_DIR}/fidl/Connectivity.fidl
        COMMAND ${COMMONAPI_CORE_GENERATOR} -sk ${CMAKE_CURRENT_SOURCE_DIR}/fidl/Diagnostics.fidl
        COMMAND ${COMMONAPI_CORE_GENERATOR} -sk ${CMAKE_CURRENT_SOURCE_DIR}/fidl/VehicleSignal.fidl
        COMMAND ${COMMONAPI_SOMEIP_GENERATOR}
            -sp ${CMAKE_CURRENT_SOURCE_DIR}/fidl
            -d ${GEN_ROOT}
            -dp ${GEN_PROXY}
            -ds ${GEN_STUB}
            ${CMAKE_CURRENT_SOURCE_DIR}/fidl/ConnectedVehiclePlatform.fdepl
        COMMAND ${CMAKE_COMMAND} -E touch ${GEN_ROOT}/.stamp
        DEPENDS ${FIDL_FILES}
        COMMENT "Generating CommonAPI sources"
    )

    add_custom_target(commonapi_codegen DEPENDS ${GEN_ROOT}/.stamp)
endif()

# -----------------------------
# FetchContent Dependencies
# -----------------------------
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
)
FetchContent_MakeAvailable(json)

# -----------------------------
# Helper function for libraries
# -----------------------------
function(add_cvp_library name)
    add_library(${name} ${ARGN})
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
endfunction()

# -----------------------------
# Libraries
# -----------------------------
add_cvp_library(logger src/Logger/logger.cpp)
add_cvp_library(auth_client src/AuthClient/auth_client.cpp)
add_cvp_library(mqtt_client src/MqttClient/mqtt_client.cpp)
target_link_libraries(mqtt_client PUBLIC ${CVP_PAHO_MQTT_TARGET} Threads::Threads)

add_cvp_library(telemetry_manager src/TelemetryManager/telemetry_manager.cpp)
target_link_libraries(telemetry_manager PUBLIC mqtt_client)

add_cvp_library(someip_client src/SomeipClient/someip_client.cpp)

add_cvp_library(adaptive_autosar_services
    src/auth/auth_session.cpp
    src/connectivity/connectivity_runtime.cpp
    src/mqtt/mqtt_gateway.cpp
    src/services/diagnostics_service.cpp
    src/services/vehicle_signal_service.cpp
)

if(CVP_ENABLE_COMMONAPI_CODEGEN)
    add_dependencies(adaptive_autosar_services commonapi_codegen)
    target_include_directories(adaptive_autosar_services PUBLIC
        ${GEN_COMMONAPI} ${GEN_PROXY} ${GEN_STUB} ${GEN_SRC_GEN}
    )
endif()

add_cvp_library(connectivity_service src/ConnectivityService/connectivity_service.cpp)
target_link_libraries(connectivity_service
    PUBLIC auth_client mqtt_client telemetry_manager logger
)

# -----------------------------
# Executables
# -----------------------------
add_executable(cvp_app src/main.cpp)
target_link_libraries(cvp_app PRIVATE connectivity_service someip_client nlohmann_json::nlohmann_json)

# -----------------------------
# Tests
# -----------------------------
if(BUILD_TESTING)
    add_executable(cvp_tests
        tests/connectivity_tests.cpp
        tests/someip_tests.cpp
        tests/adaptive_autosar_tests.cpp
    )
    target_link_libraries(cvp_tests PRIVATE
        connectivity_service someip_client adaptive_autosar_services
        GTest::gtest_main
    )
    add_test(NAME cvp_tests COMMAND cvp_tests)
endif()
