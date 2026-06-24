cmake_minimum_required(VERSION 3.21)

project(ConnectedVehiclePlatform LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
include(FetchContent)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_compile_options(-Wall -Wextra -Wpedantic)

option(CVP_USE_SYSTEM_PAHO "Use system-installed Paho MQTT libraries instead of FetchContent" OFF)
option(CVP_ENABLE_COMMONAPI_CODEGEN "Regenerate CommonAPI/SOME/IP sources from FIDL (requires external generators)" OFF)

find_package(Threads REQUIRED)

if(CVP_USE_SYSTEM_PAHO)
  find_package(PahoMqttCpp REQUIRED)
  set(CVP_PAHO_MQTT_TARGET PahoMqttCpp::paho-mqttpp3)
else()
  include(FetchPahoMqtt)
endif()

set(GENERATED_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated)
set(GENERATED_COMMONAPI_DIR ${GENERATED_ROOT_DIR}/v1)
set(GENERATED_PROXY_DIR ${GENERATED_ROOT_DIR}/proxy)
set(GENERATED_STUB_DIR ${GENERATED_ROOT_DIR}/stub)
set(GENERATED_SRC_GEN_DIR ${GENERATED_ROOT_DIR}/src-gen/v1)

if(CVP_ENABLE_COMMONAPI_CODEGEN)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  file(GLOB FIDL_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fidl/*.fidl)
  find_package(CommonAPI REQUIRED)
  if(NOT COMMONAPI_CORE_GENERATOR OR NOT COMMONAPI_SOMEIP_GENERATOR)
    message(FATAL_ERROR
      "CommonAPI generators were not found. Install commonapi-core-generator and "
      "commonapi-someip-generator, add them to PATH, or set COMMONAPI_HOME.")
  endif()

  file(MAKE_DIRECTORY
    ${GENERATED_COMMONAPI_DIR}
    ${GENERATED_PROXY_DIR}
    ${GENERATED_STUB_DIR}
    ${GENERATED_SRC_GEN_DIR}
  )

  add_custom_command(
    OUTPUT ${GENERATED_ROOT_DIR}/.stamp
    COMMAND ${CMAKE_COMMAND} -E make_directory
      ${GENERATED_COMMONAPI_DIR}
      ${GENERATED_PROXY_DIR}
      ${GENERATED_STUB_DIR}
      ${GENERATED_SRC_GEN_DIR}
    COMMAND ${COMMONAPI_CORE_GENERATOR} -sk ${CMAKE_CURRENT_SOURCE_DIR}/fidl/Connectivity.fidl
    COMMAND ${COMMONAPI_CORE_GENERATOR} -sk ${CMAKE_CURRENT_SOURCE_DIR}/fidl/Diagnostics.fidl
    COMMAND ${COMMONAPI_CORE_GENERATOR} -sk ${CMAKE_CURRENT_SOURCE_DIR}/fidl/VehicleSignal.fidl
    COMMAND ${COMMONAPI_SOMEIP_GENERATOR}
      -sp ${CMAKE_CURRENT_SOURCE_DIR}/fidl
      -d ${GENERATED_ROOT_DIR}
      -dp ${GENERATED_PROXY_DIR}
      -ds ${GENERATED_STUB_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/fidl/ConnectedVehiclePlatform.fdepl
    COMMAND ${CMAKE_COMMAND} -E rm -f
      ${GENERATED_STUB_DIR}/v1/connectivity/ConnectivityServiceSomeIPStubAdapterCatalog.json
      ${GENERATED_STUB_DIR}/v1/diagnostics/DiagnosticsServiceSomeIPStubAdapterCatalog.json
      ${GENERATED_STUB_DIR}/v1/vehicle/VehicleSignalServiceSomeIPStubAdapterCatalog.json
    COMMAND ${CMAKE_COMMAND} -E touch ${GENERATED_ROOT_DIR}/.stamp
    WORKING_DIRECTORY ${GENERATED_ROOT_DIR}
    DEPENDS ${FIDL_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/fidl/ConnectedVehiclePlatform.fdepl
    COMMENT "Generating CommonAPI C++ headers and stubs"
  )

  add_custom_target(commonapi_codegen DEPENDS ${GENERATED_ROOT_DIR}/.stamp)
endif()

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

add_library(logger src/Logger/logger.cpp)
target_include_directories(logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(logger PUBLIC cxx_std_20)

add_library(auth_client src/AuthClient/auth_client.cpp)
target_include_directories(auth_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(auth_client PUBLIC cxx_std_20)

add_library(mqtt_client src/MqttClient/mqtt_client.cpp)
target_include_directories(mqtt_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(mqtt_client PUBLIC cxx_std_20)
target_link_libraries(mqtt_client PUBLIC ${CVP_PAHO_MQTT_TARGET} Threads::Threads)

add_library(telemetry_manager src/TelemetryManager/telemetry_manager.cpp)
target_include_directories(telemetry_manager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(telemetry_manager PUBLIC cxx_std_20)
target_link_libraries(telemetry_manager PUBLIC mqtt_client)

add_library(someip_client src/SomeipClient/someip_client.cpp)
target_include_directories(someip_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(someip_client PUBLIC cxx_std_20)

add_library(adaptive_autosar_services
  src/auth/auth_session.cpp
  src/connectivity/connectivity_runtime.cpp
  src/mqtt/mqtt_gateway.cpp
  src/services/diagnostics_service.cpp
  src/services/vehicle_signal_service.cpp)
target_include_directories(adaptive_autosar_services PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(adaptive_autosar_services PUBLIC cxx_std_20)
if(CVP_ENABLE_COMMONAPI_CODEGEN)
  add_dependencies(adaptive_autosar_services commonapi_codegen)
  target_include_directories(adaptive_autosar_services PUBLIC
    ${GENERATED_COMMONAPI_DIR}
    ${GENERATED_PROXY_DIR}
    ${GENERATED_STUB_DIR}
    ${GENERATED_SRC_GEN_DIR}
  )
endif()

add_library(connectivity_service src/ConnectivityService/connectivity_service.cpp)
target_include_directories(connectivity_service PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(connectivity_service PUBLIC cxx_std_20)
target_link_libraries(connectivity_service
  PUBLIC
    auth_client
    mqtt_client
    telemetry_manager
    logger
)

add_executable(cvp_app src/main.cpp)
target_link_libraries(cvp_app PRIVATE connectivity_service someip_client nlohmann_json::nlohmann_json)

if(BUILD_TESTING)
  add_executable(cvp_tests
    tests/connectivity_tests.cpp
    tests/someip_tests.cpp
    tests/adaptive_autosar_tests.cpp)
  target_link_libraries(cvp_tests PRIVATE connectivity_service someip_client adaptive_autosar_services GTest::gtest_main)
  add_test(NAME cvp_tests COMMAND cvp_tests)
endif()
