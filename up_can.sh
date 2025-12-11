#!/bin/bash
# Configuración específica para Innodisk EMUC-B202
# CH1 (emuccan0): Motors @ 1 Mbps (Speed 9)
# CH2 (emuccan1): BMS @ 500 Kbps (Speed 7)

echo "=== Reiniciando EMUC Daemon ==="
echo 'FOX' | sudo -S killall emucd_64
sleep 2

echo "Iniciando daemon con -s97 (1M/500k)..."
# ttyACM0 es el dispositivo serial virtual del EMUC
echo 'FOX' | sudo -S /usr/sbin/emucd_64 -s97 -e0 ttyACM0 emuccan0 emuccan1 emuccan2 emuccan3

sleep 3

echo "=== Configurando cola de transmisión ==="
echo 'FOX' | sudo -S ip link set emuccan0 txqueuelen 1000
echo 'FOX' | sudo -S ip link set emuccan0 up
echo 'FOX' | sudo -S ip link set emuccan1 txqueuelen 1000
echo 'FOX' | sudo -S ip link set emuccan1 up

echo "=== Estado final ==="
ip link show emuccan0
ip link show emuccan1
