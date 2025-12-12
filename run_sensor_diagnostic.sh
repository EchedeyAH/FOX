#!/bin/bash
# Script para compilar y ejecutar el diagnóstico de sensores en la ECU

ECU_HOST="fox@193.147.165.236"
ECU_DIR="/home/fox/ecu_atc8110"

echo "=== Diagnóstico de Sensores ECU FOX ==="
echo ""
echo "📡 Conectando a ECU ($ECU_HOST)..."

# Compilar el diagnóstico en la ECU
ssh $ECU_HOST << 'ENDSSH'
cd /home/fox/ecu_atc8110/build
echo "🔨 Compilando sensor_diagnostic..."
cmake .. -DCMAKE_BUILD_TYPE=Debug > /dev/null 2>&1
make sensor_diagnostic

if [ $? -eq 0 ]; then
    echo "✅ Compilación exitosa"
    echo ""
    echo "🔍 Ejecutando diagnóstico de sensores..."
    echo "==========================================="
    sudo ./tools/sensor_diagnostic
else
    echo "❌ Error en la compilación"
    exit 1
fi
ENDSSH

echo ""
echo "✅ Diagnóstico completado"
