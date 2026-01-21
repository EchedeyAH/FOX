#!/bin/bash
#
# Script de inicio automático de interfaces CAN para FOX ECU
# Este script se ejecuta al arranque del sistema vía systemd
#

set -e

LOG_FILE="/var/log/fox_can_startup.log"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

log "========================================="
log "  FOX ECU - CAN Interface Startup"
log "========================================="

# Matar cualquier instancia previa del daemon
log "Limpiando procesos previos..."
pkill -9 emucd_64 2>/dev/null || true
sleep 2

# Detectar el puerto USB correcto (ttyACM0, ttyACM1, ttyACM2, etc.)
EMUC_PORT=""
for port in /dev/ttyACM{0..3}; do
    if [ -e "$port" ]; then
        log "Puerto detectado: $port"
        EMUC_PORT=$(basename "$port")
        break
    fi
done

if [ -z "$EMUC_PORT" ]; then
    log "ERROR: No se encontró ningún puerto ttyACM*"
    exit 1
fi

log "Usando puerto: $EMUC_PORT"

# Iniciar el daemon EMUC
# -s8 = 1 Mbps para emuccan0 (Motores)
# -s7 = 500 Kbps (si el driver lo soporta, sino usar -s8 para ambos)
# Nota: Algunos drivers EMUC usan un solo -s para todos los puertos
log "Iniciando daemon emucd_64..."
/usr/sbin/emucd_64 -s8 -e0 "$EMUC_PORT" emuccan0 emuccan1 emuccan2 emuccan3 > /tmp/emuc_daemon.log 2>&1 &
DAEMON_PID=$!

sleep 3

# Verificar que el daemon sigue corriendo
if ! ps -p $DAEMON_PID > /dev/null 2>&1; then
    log "ERROR: El daemon emucd_64 falló al iniciar"
    log "Log del daemon:"
    cat /tmp/emuc_daemon.log | tee -a "$LOG_FILE"
    exit 1
fi

log "Daemon iniciado correctamente (PID: $DAEMON_PID)"

# Configurar emuccan0 (Motores - 1 Mbps)
log "Configurando emuccan0 (Motores)..."
if ip link show emuccan0 > /dev/null 2>&1; then
    ip link set emuccan0 down 2>/dev/null || true
    ip link set emuccan0 txqueuelen 1000
    ip link set emuccan0 up
    log "✓ emuccan0 configurado y activo"
else
    log "⚠ emuccan0 no disponible"
fi

# Configurar emuccan1 (BMS - 500 Kbps)
log "Configurando emuccan1 (BMS)..."
if ip link show emuccan1 > /dev/null 2>&1; then
    ip link set emuccan1 down 2>/dev/null || true
    ip link set emuccan1 txqueuelen 1000
    ip link set emuccan1 up
    log "✓ emuccan1 configurado y activo"
else
    log "⚠ emuccan1 no disponible"
fi

# Verificar estado final
log ""
log "Estado de interfaces CAN:"
ip -details link show emuccan0 2>/dev/null | tee -a "$LOG_FILE" || log "emuccan0: No disponible"
ip -details link show emuccan1 2>/dev/null | tee -a "$LOG_FILE" || log "emuccan1: No disponible"

log ""
log "Configuración CAN completada exitosamente"
log "========================================="

exit 0
