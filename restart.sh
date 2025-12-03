#!/bin/bash
# Script de reinicio de la ECU

# Detener proceso anterior si existe
pkill -9 ecu_atc8110 || true

# Esperar un momento
sleep 2

# Configurar interfaces CAN
sudo /home/fox/ecu_app/bin/setup_can.sh --real

# Iniciar nueva versión
cd /home/fox/ecu_app/bin
nohup sudo ./ecu_atc8110 > ../logs/ecu_$(date +%Y%m%d_%H%M%S).log 2>&1 &

echo "ECU reiniciada. PID: $!"
