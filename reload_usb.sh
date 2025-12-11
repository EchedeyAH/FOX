#!/bin/bash
# Recargar drivers USB serial

echo "Recargando cdc_acm..."
echo 'FOX' | sudo -S modprobe -r cdc_acm
sleep 1
echo 'FOX' | sudo -S modprobe cdc_acm
sleep 3
echo "Dispositivos detectados:"
ls -l /dev/ttyACM*
