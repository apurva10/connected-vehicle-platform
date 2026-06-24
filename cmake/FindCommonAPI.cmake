# Optional CommonAPI code generators (only needed when CVP_ENABLE_COMMONAPI_CODEGEN=ON).
# Set COMMONAPI_HOME or the COMMONAPI_HOME environment variable to the generator install root.

set(_COMMONAPI_HINTS "")
if(DEFINED ENV{COMMONAPI_HOME})
  list(APPEND _COMMONAPI_HINTS
    "$ENV{COMMONAPI_HOME}/bin"
    "$ENV{COMMONAPI_HOME}/tools"
    "$ENV{COMMONAPI_HOME}/core-generator"
    "$ENV{COMMONAPI_HOME}/someip-generator")
endif()
if(DEFINED COMMONAPI_HOME)
  list(APPEND _COMMONAPI_HINTS
    "${COMMONAPI_HOME}/bin"
    "${COMMONAPI_HOME}/tools"
    "${COMMONAPI_HOME}/core-generator"
    "${COMMONAPI_HOME}/someip-generator")
endif()

find_program(COMMONAPI_CORE_GENERATOR
  NAMES commonapi-core-generator-linux-x86_64 commonapi-core-generator
  HINTS ${_COMMONAPI_HINTS}
  PATHS /usr/local/bin /usr/bin /opt/commonapi/bin)

find_program(COMMONAPI_SOMEIP_GENERATOR
  NAMES commonapi-someip-generator-linux-x86_64 commonapi-someip-generator
  HINTS ${_COMMONAPI_HINTS}
  PATHS /usr/local/bin /usr/bin /opt/commonapi/bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CommonAPI
  REQUIRED_VARS COMMONAPI_CORE_GENERATOR COMMONAPI_SOMEIP_GENERATOR)

mark_as_advanced(COMMONAPI_CORE_GENERATOR COMMONAPI_SOMEIP_GENERATOR)
