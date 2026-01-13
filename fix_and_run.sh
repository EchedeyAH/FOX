#!/bin/bash
# fix_and_run.sh - REPARACIÓN TOTAL Y ARRANQUE DE ECU FOX

echo "===================================================="
echo "   REPARACIÓN DE ENTORNO Y ARRANQUE - ECU FOX       "
echo "===================================================="

# 1. Levantar CAN
echo "1. Configurando interfaces CAN (Auto-detect)..."
chmod +x auto_config_can.sh
./auto_config_can.sh
if [ $? -ne 0 ]; then
    echo "ERROR CRÍTICO: No se pudo levantar CAN. Abortando."
    exit 1
fi

# Esperar un poco a que las interfaces suban
sleep 2

# 2. Limpiar y Recompilar
echo "2. Recompilando software (limpieza completa)..."
cd ecu_atc8110
rm -rf build
mkdir build && cd build
cmake ..
make -j4
cd ../..

# 3. Lanzar
echo "3. Iniciando ECU con privilegios..."
echo "----------------------------------------------------"
echo "CONSEJO: En otra ventana pisa el freno o usa:"
echo "echo 1 > /tmp/force_brake"
echo "----------------------------------------------------"

# Ejecutar ECU
sudo ./ecu_atc8110/build/ecu_atc8110
