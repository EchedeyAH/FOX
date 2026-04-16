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

# Detectar puertos USB disponibles (ttyACM0, ttyACM1, ...)
EMUC_PORTS=()
for port in /dev/ttyACM{0..3}; do
    if [ -e "$port" ]; then
        log "Puerto detectado: $port"
        EMUC_PORTS+=("$(basename "$port")")
    fi
done

if [ ${#EMUC_PORTS[@]} -eq 0 ]; then
    log "ERROR: No se encontró ningún puerto ttyACM*"
    exit 1
fi

DAEMON_PIDS=()

start_emucd() {
    local port="$1"
    local if0="$2"
    local if1="$3"
    local daemon_log="$4"

    log "Iniciando daemon emucd_64 en ${port} -> ${if0},${if1} ..."
    /usr/sbin/emucd_64 -s8 -e0 "$port" "$if0" "$if1" > "$daemon_log" 2>&1 &
    local pid=$!
    sleep 2

    if ps -p "$pid" > /dev/null 2>&1; then
        log "Daemon iniciado correctamente (PID: $pid)"
        DAEMON_PIDS+=("$pid")
    else
        log "ERROR: emucd_64 falló en ${port} para ${if0},${if1}"
        log "Log ${daemon_log}:"
        cat "$daemon_log" | tee -a "$LOG_FILE"
        return 1
    fi
}

# Puerto 0 -> emuccan0/1 (BMS/IMU)
start_emucd "${EMUC_PORTS[0]}" emuccan0 emuccan1 /tmp/emuc_daemon_0.log

# Puerto 1 -> emuccan2/3 (MOTORES y extra), si existe
if [ ${#EMUC_PORTS[@]} -ge 2 ]; then
    start_emucd "${EMUC_PORTS[1]}" emuccan2 emuccan3 /tmp/emuc_daemon_1.log
else
    log "⚠ Solo hay un puerto ttyACM disponible; emuccan2/3 pueden no existir"
fi

# Configurar emuccan0 (BMS en este mapeo)
log "Configurando emuccan0 (BMS)..."
if ip link show emuccan0 > /dev/null 2>&1; then
    ip link set emuccan0 down 2>/dev/null || true
    ip link set emuccan0 txqueuelen 1000
    ip link set emuccan0 up
    log "✓ emuccan0 configurado y activo"
else
    log "⚠ emuccan0 no disponible"
fi

# Configurar emuccan1 (IMU en este mapeo)
log "Configurando emuccan1 (IMU)..."
if ip link show emuccan1 > /dev/null 2>&1; then
    ip link set emuccan1 down 2>/dev/null || true
    ip link set emuccan1 txqueuelen 1000
    ip link set emuccan1 up
    log "✓ emuccan1 configurado y activo"
else
    log "⚠ emuccan1 no disponible"
fi

# Configurar emuccan2 (Motores)
log "Configurando emuccan2 (Motores)..."
if ip link show emuccan2 > /dev/null 2>&1; then
    ip link set emuccan2 down 2>/dev/null || true
    ip link set emuccan2 txqueuelen 1000
    ip link set emuccan2 up
    log "✓ emuccan2 configurado y activo"
else
    log "⚠ emuccan2 no disponible"
fi

# Verificar estado final
log ""
log "Estado de interfaces CAN:"
ip -details link show emuccan0 2>/dev/null | tee -a "$LOG_FILE" || log "emuccan0: No disponible"
ip -details link show emuccan1 2>/dev/null | tee -a "$LOG_FILE" || log "emuccan1: No disponible"
ip -details link show emuccan2 2>/dev/null | tee -a "$LOG_FILE" || log "emuccan2: No disponible"

log ""
log "Configuración CAN completada exitosamente"
log "========================================="

exit 0
