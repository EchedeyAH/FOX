# CMakeLists.txt para herramientas de diagnóstico

# Herramienta de diagnóstico CAN
add_executable(can_diagnostic
    can_diagnostic.cpp
)

link_ecu_libs(can_diagnostic)

target_include_directories(can_diagnostic
    PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Herramienta de diagnóstico de sensores
add_executable(sensor_diagnostic
    sensor_diagnostic.cpp
)

link_ecu_libs(sensor_diagnostic)

target_include_directories(sensor_diagnostic
    PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Instalar en directorio tools
install(TARGETS can_diagnostic sensor_diagnostic
    RUNTIME DESTINATION tools
)

