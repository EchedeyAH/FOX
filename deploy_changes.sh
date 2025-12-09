#!/bin/bash
# Script para transferir archivos modificados al ECU

ECU_IP="193.147.165.236"
ECU_USER="fox"
LOCAL_DIR="ecu_atc8110"
REMOTE_DIR="/home/fox/ecu_atc8110"

echo "=== Transferencia de archivos al ECU ==="
echo ""

# Transferir solo los archivos modificados
echo "1. Transfiriendo CMakeLists.txt..."
scp ${LOCAL_DIR}/CMakeLists.txt ${ECU_USER}@${ECU_IP}:${REMOTE_DIR}/

echo "2. Transfiriendo traction_control_wrapper.cpp..."
scp ${LOCAL_DIR}/control_vehiculo/traction_control_wrapper.cpp ${ECU_USER}@${ECU_IP}:${REMOTE_DIR}/control_vehiculo/

echo ""
echo "=== Transferencia completada ==="
echo ""
echo "Ahora puedes compilar en el ECU con:"
echo "  ssh ${ECU_USER}@${ECU_IP}"
echo "  cd ${REMOTE_DIR}"
echo "  rm -rf build && mkdir build && cd build"
echo "  cmake .. && make -j4"
