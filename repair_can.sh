#!/bin/bash
# Script de recuperación de interfaces CAN para FOX ECU

# 1. Detener procesos existentes
echo "Deteniendo daemon emucd_64..."
echo 'FOX' | sudo -S pkill -9 emucd_64
sleep 2

# 2. Iniciar daemon manualmente con configuración correcta
# ttyACM0 @ 500kbps (7)
echo "Iniciando emucd_64 en ttyACM0..."
echo 'FOX' | sudo -S nohup /usr/sbin/emucd_64 -s7 -e0 ttyACM0 emuccan0 emuccan1 emuccan2 emuccan3 > /tmp/emuc.log 2>&1 &
sleep 3

# 3. Configurar y levantar interfaces
echo "Levantando interfaces..."

# Motor (emuccan0)
echo 'FOX' | sudo -S ip link set emuccan0 txqueuelen 1000
echo 'FOX' | sudo -S ip link set emuccan0 up

# BMS (emuccan1)
echo 'FOX' | sudo -S ip link set emuccan1 txqueuelen 1000
echo 'FOX' | sudo -S ip link set emuccan1 up

# 4. Verificación
echo "=== Estado Final ==="
ps aux | grep emucd_64 | grep -v grep
echo "-------------------"
ip link show | grep emuccan
