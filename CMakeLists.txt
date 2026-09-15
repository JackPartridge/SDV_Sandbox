cmake_minimum_required(VERSION 3.20)
project(sdvSandbox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
enable_testing()

if(DEFINED ENV{VECU_VSOMEIP_PREFIX} AND NOT CMAKE_PREFIX_PATH)
  list(APPEND CMAKE_PREFIX_PATH "$ENV{VECU_VSOMEIP_PREFIX}")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/vsomeip-prefix")
  list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/vsomeip-prefix")
endif()

find_package(vsomeip3 REQUIRED)

add_library(sdvCommon
  src/common/sensorReading.cpp
  src/common/sensorSimulator.cpp
  src/common/telemetryFrame.cpp
  src/common/hardwareCollector.cpp
  src/common/canFrame.cpp
  src/common/canBusSimulator.cpp
  src/common/vehicleSignalFrame.cpp
  src/common/platformPaths.cpp
)
target_include_directories(sdvCommon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/common")

add_library(vecu_sdk SHARED
  src/sdk/vecu_sdk.cpp
  src/sdk/vecuServiceConfig.cpp
)
target_include_directories(vecu_sdk
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/sdk>"
    "$<INSTALL_INTERFACE:include>"
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(vecu_sdk
  PRIVATE
    vsomeip3
    sdvCommon
)
if(WIN32)
  target_link_libraries(vecu_sdk PRIVATE ws2_32)
endif()
target_compile_definitions(vecu_sdk
  PRIVATE
    VECU_SDK_BUILD
)
set_target_properties(vecu_sdk PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  VERSION 1.0.0
  SOVERSION 1
  OUTPUT_NAME vecu_sdk
)

add_executable(helloWorld
  src/helloWorld/main.cpp
)

add_executable(sensorPublisher
  src/publisher/main.cpp
  src/publisher/sensorPublisher.cpp
)
target_include_directories(sensorPublisher PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(sensorPublisher PRIVATE sdvCommon vsomeip3)
if(WIN32)
  target_link_libraries(sensorPublisher PRIVATE ws2_32)
endif()

add_executable(sensorSubscriber
  src/subscriber/main.cpp
  src/subscriber/sensorSubscriber.cpp
)
target_include_directories(sensorSubscriber PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(sensorSubscriber PRIVATE sdvCommon vsomeip3)
if(WIN32)
  target_link_libraries(sensorSubscriber PRIVATE ws2_32)
endif()

add_executable(canGateway
  src/canGateway/main.cpp
)
target_include_directories(canGateway PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/common")
target_link_libraries(canGateway PRIVATE vecu_sdk sdvCommon)
if(WIN32)
  target_link_libraries(canGateway PRIVATE ws2_32)
endif()

add_executable(ciContractHarness
  src/ciHarness/main.cpp
)
target_include_directories(ciContractHarness PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/common")
target_link_libraries(ciContractHarness PRIVATE vecu_sdk sdvCommon)
if(WIN32)
  target_link_libraries(ciContractHarness PRIVATE ws2_32)
endif()

add_executable(exampleFetcher
  examples/example_fetcher.cpp
)
target_link_libraries(exampleFetcher PRIVATE vecu_sdk)
if(WIN32)
  target_link_libraries(exampleFetcher PRIVATE ws2_32)
endif()

set(_vecu_runtime_targets canGateway ciContractHarness exampleFetcher)
foreach(_target IN LISTS _vecu_runtime_targets)
  set_target_properties(${_target} PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
  )
endforeach()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(sdvUnitTests
  tests/sensorReadingTests.cpp
  tests/sensorSimulatorTests.cpp
  tests/telemetryFrameTests.cpp
  tests/canFrameTests.cpp
  tests/vecuServiceConfigTests.cpp
)
target_link_libraries(sdvUnitTests PRIVATE sdvCommon vecu_sdk GTest::gtest_main)
if(WIN32)
  target_link_libraries(sdvUnitTests PRIVATE ws2_32)
endif()
set_target_properties(sdvUnitTests PROPERTIES
  BUILD_RPATH "$ORIGIN"
  INSTALL_RPATH "$ORIGIN"
)
include(GoogleTest)
gtest_discover_tests(sdvUnitTests)
