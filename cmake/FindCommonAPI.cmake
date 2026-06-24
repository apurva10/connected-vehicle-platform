find_program(COMMONAPI_CORE_GENERATOR NAMES commonapi-core-generator-linux-x86_64 PATHS
  /usr/local/bin
  /usr/bin
  /opt/commonapi/bin
  /home/apurva/commonapi/tools
  /home/apurva/commonapi/core-generator
  NO_DEFAULT_PATH)

find_program(COMMONAPI_SOMEIP_GENERATOR NAMES commonapi-someip-generator-linux-x86_64 PATHS
  /usr/local/bin
  /usr/bin
  /opt/commonapi/bin
  /home/apurva/commonapi/tools
  /home/apurva/commonapi/someip-generator
  NO_DEFAULT_PATH)

if(NOT COMMONAPI_CORE_GENERATOR)
  find_program(COMMONAPI_CORE_GENERATOR NAMES commonapi-core-generator-linux-x86_64)
endif()

if(NOT COMMONAPI_SOMEIP_GENERATOR)
  find_program(COMMONAPI_SOMEIP_GENERATOR NAMES commonapi-someip-generator-linux-x86_64)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CommonAPI
  REQUIRED_VARS COMMONAPI_CORE_GENERATOR COMMONAPI_SOMEIP_GENERATOR)

mark_as_advanced(COMMONAPI_CORE_GENERATOR COMMONAPI_SOMEIP_GENERATOR)
