#!/bin/bash
# Script de prueba continua para motor trasero 1 (ID 0x201)
# Envia comando de Throttle=10% Brake=0% cada 50ms

echo "Enviando comandos a Motor 1 (0x201)... CTRL+C para parar"

while true; do
    # ID 201, Data: 10 00 (Throttle, Brake)
    cansend emuccan0 201#1000
    sleep 0.05
done
