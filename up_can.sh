#!/bin/bash
# Configuración específica para Innodisk EMUC-B202
# CH1 (emuccan0): Motors @ 1 Mbps (Speed 9)
# CH2 (emuccan1): BMS @ 500 Kbps (Speed 7)

echo "=== Reiniciando EMUC Daemon ==="
echo 'FOX' | sudo -S killall emucd_64
sleep 2

echo "Iniciando daemons en ttyACM1 y ttyACM2 (-s97)..."
# ttyACM1: emuccan0 (1M), emuccan1 (500k)
echo 'FOX' | sudo -S /usr/sbin/emucd_64 -s97 -e0 ttyACM1 emuccan0 emuccan1 > /tmp/emuc_acm1.log 2>&1 &
# ttyACM2: emuccan2 (1M?), emuccan3 (500k?)
echo 'FOX' | sudo -S /usr/sbin/emucd_64 -s97 -e0 ttyACM2 emuccan2 emuccan3 > /tmp/emuc_acm2.log 2>&1 &

sleep 3

echo "=== Configurando interfaces (UP) ==="
for i in 0 1 2 3; do
    echo "Activando emuccan$i..."
    echo 'FOX' | sudo -S ip link set emuccan$i txqueuelen 1000
    echo 'FOX' | sudo -S ip link set emuccan$i up
done

echo "=== Estado final ==="
ip link show emuccan0
ip link show emuccan1
