cmake_minimum_required(VERSION 3.10)
project(ECU_FOX C)

set(CMAKE_C_STANDARD 11)

# Directorios de cabeceras
include_directories(${CMAKE_SOURCE_DIR}/include/ecu_fox)
include_directories(${CMAKE_SOURCE_DIR}/include/third_party)

# Directorio de librerías externas
link_directories(${CMAKE_SOURCE_DIR}/libs)

# Recopilamos todos los ficheros fuente de src/
file(GLOB_RECURSE SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.c)

# Creamos el ejecutable principal
add_executable(ecu_fox ${SRC_FILES})

# Selecciona la librería adecuada según la arquitectura
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PCI_LIB "libpci_64.a")
else()
    set(PCI_LIB "libpci.a")
endif()

# Enlaza la librería del fabricante
target_link_libraries(ecu_fox ${PCI_LIB})
