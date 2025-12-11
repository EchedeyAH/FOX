#!/bin/bash
# Levantar interfaces CAN
echo "Levantando emuccan0..."
echo 'FOX' | sudo -S ip link set emuccan0 txqueuelen 1000
echo 'FOX' | sudo -S ip link set emuccan0 up

echo "Levantando emuccan1..."
echo 'FOX' | sudo -S ip link set emuccan1 txqueuelen 1000
echo 'FOX' | sudo -S ip link set emuccan1 up

echo "=== Estado interfaces ==="
ip link show | grep emuccan
