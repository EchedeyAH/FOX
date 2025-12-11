#!/bin/bash
# Script de recuperación robusto para CAN

# Crear el script interno que se ejecutará como root
cat > /tmp/init_can_root.sh << 'EOF'
#!/bin/bash
pkill -9 emucd_64
sleep 2

echo "Iniciando daemon..."
# Ejecutar en background y guardar PID
/usr/sbin/emucd_64 -s7 -e0 ttyACM0 emuccan0 emuccan1 emuccan2 emuccan3 > /tmp/emuc_debug.log 2>&1 &
PID=$!
echo "Daemon iniciado con PID $PID"
sleep 3

# Verificar si el proceso sigue corriendo
if ps -p $PID > /dev/null; then
    echo "Daemon corriendo, configurando interfaces..."
    ip link set emuccan0 txqueuelen 1000
    ip link set emuccan0 up
    ip link set emuccan1 txqueuelen 1000
    ip link set emuccan1 up
    echo "Configuración completada"
else
    echo "CRITICAL: El daemon murió inmediatamente. Log:"
    cat /tmp/emuc_debug.log
    exit 1
fi
EOF

chmod +x /tmp/init_can_root.sh

# Ejecutar el script interno con sudo
echo "Ejecutando script como root..."
echo 'FOX' | sudo -S /tmp/init_can_root.sh

# Verificación final
echo "=== Verificación ==="
ip link show | grep emuccan
ps aux | grep emucd_64 | grep -v grep
