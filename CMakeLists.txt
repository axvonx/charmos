cmake_minimum_required(VERSION 3.19)
project(charmOS C ASM_NASM)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(ARCH "x86_64")
set(IMAGE_NAME "charmos-${ARCH}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE
        "Debug"
        CACHE STRING "Build type: Debug, Release, RelWithDebInfo, MinSizeRel" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif ()

message(STATUS "charmOS build type: ${CMAKE_BUILD_TYPE}")

include(features) # compile-time feature toggles
include(tests) # test flags
include(flags) # compiler / linker / codegen

add_subdirectory(kernel)
target_include_directories(kernel PRIVATE include)

set(CHARMOS_COMMIT
    ""
    CACHE STRING "Commit identity for the kernel")
set(CHARMOS_SOURCE_ROOT
    "${CMAKE_SOURCE_DIR}"
    CACHE PATH "Local source root used in terminal hyperlinks emitted by kernel tests")
if (CHARMOS_COMMIT STREQUAL "")
    execute_process(
        COMMAND git rev-parse --short=12 HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE CHARMOS_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
endif ()
if (CHARMOS_COMMIT STREQUAL "")
    set(CHARMOS_COMMIT "unknown")
endif ()
target_compile_definitions(kernel PRIVATE CHARMOS_COMMIT="${CHARMOS_COMMIT}"
                                          CHARMOS_SOURCE_ROOT="${CHARMOS_SOURCE_ROOT}")

include(emulation)

add_custom_target(
    clean-full
    COMMAND ${CMAKE_COMMAND} -E rm -rf iso_root ${IMAGE_NAME}.iso ${IMAGE_NAME}.hdd kernel-deps limine ovmf
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

message(STATUS "")
message(STATUS "charmOS configuration:")
message(STATUS "  Build type   : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Arch         : ${ARCH}")
message(STATUS "  Stack usage  : ${KERNEL_STACK_USAGE}")
test_report_configuration()
message(STATUS "  Profiling    : ${PROFILING_ENABLED} (PROFILING_ALL=${PROFILING_ALL})")
machine_report_configuration()
message(STATUS "")
