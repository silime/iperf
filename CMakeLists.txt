cmake_minimum_required(VERSION 3.20)
project(iperf VERSION 3.21 LANGUAGES C)

if(NOT WIN32)
    message(FATAL_ERROR "This CMake build currently targets native Windows; use Autotools on Unix.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
configure_file(src/iperf_config_windows.h.in src/iperf_config.h @ONLY)
configure_file(src/version.h.in src/version.h @ONLY)

set(IPERF_SOURCES
    src/cjson.c src/dscp.c src/iperf_api.c src/iperf_client_api.c
    src/iperf_error.c src/iperf_auth.c src/iperf_locale.c src/iperf_pthread.c
    src/iperf_server_api.c src/iperf_tcp.c src/iperf_udp.c src/iperf_util.c
    src/iperf_time.c src/net.c src/tcp_info.c src/timer.c src/units.c
    src/windows/getopt.c src/windows/iperf_windows.c src/windows/pthread.c)

add_library(iperf STATIC ${IPERF_SOURCES})
set_source_files_properties(src/windows/iperf_windows.c PROPERTIES
    COMPILE_DEFINITIONS IPERF_WINDOWS_IMPLEMENTATION)
target_include_directories(iperf PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/windows/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>")
target_compile_definitions(iperf PUBLIC _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS WIN32_LEAN_AND_MEAN)
target_compile_options(iperf PRIVATE /W3 /FIiperf_windows.h)
target_link_libraries(iperf PUBLIC ws2_32 bcrypt iphlpapi)

add_executable(iperf3 src/main.c)
target_compile_options(iperf3 PRIVATE /W3 /FIiperf_windows.h)
target_link_libraries(iperf3 PRIVATE iperf)

enable_testing()
foreach(test_name IN ITEMS t_units t_timer t_uuid t_api)
    add_executable(${test_name} src/${test_name}.c)
    target_compile_options(${test_name} PRIVATE /W3 /FIiperf_windows.h)
    target_link_libraries(${test_name} PRIVATE iperf)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

install(TARGETS iperf3 RUNTIME DESTINATION bin)
