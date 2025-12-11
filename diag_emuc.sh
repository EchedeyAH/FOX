#!/bin/bash
# Script de diagnóstico para EMUC

echo "=== Comprobando ttyACM0 ==="
ls -l /dev/ttyACM0
# Ver si algo lo está usando
lsof /dev/ttyACM0
fuser /dev/ttyACM0

echo "=== Comprobando binario ==="
ls -l /usr/sbin/emucd_64
ldd /usr/sbin/emucd_64
file /usr/sbin/emucd_64

echo "=== Prueba de ejecución básica ==="
# Intentar ejecutar help para ver si el binario funciona
/usr/sbin/emucd_64 -h 2>&1 | head -5

echo "=== Logs del kernel recientes ==="
dmesg | tail -20

echo "=== Intentando ejecución directa y captura de error ==="
# Ejecutar sin background para ver output real
/usr/sbin/emucd_64 -s7 -e0 ttyACM0 emuccan0 emuccan1 emuccan2 emuccan3
echo "Exit code: $?"
