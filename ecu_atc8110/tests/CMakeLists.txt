cmake_minimum_required(VERSION 3.15)
project(ecu_tests CXX)

# ============================================================================
# CONFIGURACIÓN
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

# Directorios
set(COMMON_DIR ${CMAKE_SOURCE_DIR}/common)
set(TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)

# Incluir directorios
include_directories(
    ${COMMON_DIR}
    ${TESTS_DIR}
)

# ============================================================================
# COMPILAR TESTS
# ============================================================================

# Test runner principal v2
add_executable(ecu_tests
    ecu_tests_main_v2.cpp
    ${COMMON_DIR}/system_mode_manager.hpp
    ${COMMON_DIR}/motor_timeout_detector.hpp
    ${COMMON_DIR}/voltage_protection.hpp
    ${COMMON_DIR}/can_simulator.hpp
    ${COMMON_DIR}/telemetry_publisher.hpp
    ${COMMON_DIR}/error_catalog.hpp
    ${COMMON_DIR}/logging.hpp
)

# Linking (solo bibliotecas estándar para tests)
# En target real, linking contra libecu
target_link_libraries(ecu_tests 
    pthread
    stdc++
)

# ============================================================================
# INSTALACIÓN
# ============================================================================

install(TARGETS ecu_tests DESTINATION bin)

# ============================================================================
# TESTS CON CMAKE
# ============================================================================

enable_testing()

# Tests individuales
# add_test(NAME MotorTimeoutSuite COMMAND ecu_tests --suite=MotorTimeoutSuite)
# add_test(NAME SchedulerTimingSuite COMMAND ecu_tests --suite=SchedulerTimingSuite --timing-iterations=1000)
