#!/bin/bash
#
# Diagnostico rapido end-to-end para motores:
# - Captura trafico CAN (candump any)
# - Resume TX de comandos y RX de feedback de inversores
# - Opcional: analiza log de ECU para ver lectura de acelerador
#
# Uso:
#   sudo ./scripts/diagnose_motor_e2e.sh [duracion_s] [ruta_log_ecu]
# Ejemplo:
#   sudo ./scripts/diagnose_motor_e2e.sh 30 /tmp/ecu.log
#

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

DURATION="${1:-30}"
ECU_LOG="${2:-}"

BMS_IFACE="${BMS_IFACE:-emuccan0}"
IMU_IFACE="${IMU_IFACE:-emuccan1}"
MOTOR_IFACE_A="${MOTOR_IFACE_A:-emuccan2}"
MOTOR_IFACE_B="${MOTOR_IFACE_B:-emuccan3}"

for bin in candump ip timeout awk grep; do
    if ! command -v "${bin}" >/dev/null 2>&1; then
        echo -e "${RED}✗ Falta dependencia: ${bin}${NC}"
        exit 1
    fi
done

echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   DIAGNOSTICO E2E BUS DE MOTORES          ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════╝${NC}"
echo ""
echo "Mapeo actual:"
echo "  BMS:      ${BMS_IFACE}"
echo "  IMU:      ${IMU_IFACE}"
echo "  MOTORES:  ${MOTOR_IFACE_A}, ${MOTOR_IFACE_B}"
echo ""

for iface in "${BMS_IFACE}" "${IMU_IFACE}" "${MOTOR_IFACE_A}" "${MOTOR_IFACE_B}"; do
    if ip link show "${iface}" >/dev/null 2>&1; then
        state="$(ip -details link show "${iface}" | awk '/state/ {print $9; exit}')"
        echo -e "  ${GREEN}✓${NC} ${iface}: ${state:-UNKNOWN}"
    else
        echo -e "  ${YELLOW}⚠${NC} ${iface}: no existe"
    fi
done

echo ""
echo -e "${YELLOW}Capturando ${DURATION}s con candump any...${NC}"
echo "Durante la captura: arranca ECU y pisa acelerador 2-3 veces."
echo ""

CAN_LOG="$(mktemp /tmp/motor_e2e_can.XXXXXX.log)"
trap 'rm -f "${CAN_LOG}"' EXIT

timeout "${DURATION}" candump any > "${CAN_LOG}" 2>/dev/null || true

count_id() {
    local iface_regex="$1"
    local id_hex="$2"
    grep -E "^[[:space:]]*(${iface_regex})[[:space:]]+${id_hex}[[:space:]]" "${CAN_LOG}" | wc -l | tr -d ' '
}

IF_MOTOR_REGEX="${MOTOR_IFACE_A}|${MOTOR_IFACE_B}"

TOTAL_MOTOR="$(grep -E "^[[:space:]]*(${IF_MOTOR_REGEX})[[:space:]]+" "${CAN_LOG}" | wc -l | tr -d ' ')"
CMD_201="$(count_id "${IF_MOTOR_REGEX}" "201")"
CMD_202="$(count_id "${IF_MOTOR_REGEX}" "202")"
CMD_203="$(count_id "${IF_MOTOR_REGEX}" "203")"
CMD_204="$(count_id "${IF_MOTOR_REGEX}" "204")"
CMD_513="$(count_id "${IF_MOTOR_REGEX}" "513")"
CMD_514="$(count_id "${IF_MOTOR_REGEX}" "514")"
CMD_515="$(count_id "${IF_MOTOR_REGEX}" "515")"
CMD_516="$(count_id "${IF_MOTOR_REGEX}" "516")"
RESP_281="$(count_id "${IF_MOTOR_REGEX}" "281")"
RESP_282="$(count_id "${IF_MOTOR_REGEX}" "282")"
RESP_283="$(count_id "${IF_MOTOR_REGEX}" "283")"
RESP_284="$(count_id "${IF_MOTOR_REGEX}" "284")"

echo "Resumen CAN motores (${MOTOR_IFACE_A}/${MOTOR_IFACE_B}):"
echo "  Tramas totales en bus motores: ${TOTAL_MOTOR}"
echo "  CMD legacy (0x201..0x204): $((CMD_201 + CMD_202 + CMD_203 + CMD_204))"
echo "  CMD nuevo  (0x513..0x516): $((CMD_513 + CMD_514 + CMD_515 + CMD_516))"
echo "  RESP inv   (0x281..0x284): $((RESP_281 + RESP_282 + RESP_283 + RESP_284))"
echo ""
echo "Detalle IDs:"
echo "  0x201=${CMD_201} 0x202=${CMD_202} 0x203=${CMD_203} 0x204=${CMD_204}"
echo "  0x513=${CMD_513} 0x514=${CMD_514} 0x515=${CMD_515} 0x516=${CMD_516}"
echo "  0x281=${RESP_281} 0x282=${RESP_282} 0x283=${RESP_283} 0x284=${RESP_284}"
echo ""

if [ "${TOTAL_MOTOR}" -eq 0 ]; then
    echo -e "${RED}✗ No hay trafico en bus de motores.${NC}"
elif [ $((RESP_281 + RESP_282 + RESP_283 + RESP_284)) -eq 0 ]; then
    echo -e "${YELLOW}⚠ Hay TX de ECU pero sin respuesta de inversores.${NC}"
else
    echo -e "${GREEN}✓ Hay trafico bidireccional en bus de motores.${NC}"
fi

if [ -n "${ECU_LOG}" ] && [ -f "${ECU_LOG}" ]; then
    echo ""
    echo "Analizando log de ECU: ${ECU_LOG}"

    ACCEL_RAW_LAST="$(grep -E 'CH1 \(acelerador\): raw=' "${ECU_LOG}" | tail -1 | sed -E 's/.*raw=([0-9-]+).*/\1/' || true)"
    ACCEL_RAW_MAX="$(grep -E 'CH1 \(acelerador\): raw=' "${ECU_LOG}" | sed -E 's/.*raw=([0-9-]+).*/\1/' | sort -n | tail -1 || true)"
    ACCEL_SENSOR_MAX="$(grep -E 'Sensor Acelerador:' "${ECU_LOG}" | sed -E 's/.*Sensor Acelerador: ([0-9.]+).*/\1/' | sort -n | tail -1 || true)"

    echo "  Acelerador raw (ultimo): ${ACCEL_RAW_LAST:-N/A}"
    echo "  Acelerador raw (max):    ${ACCEL_RAW_MAX:-N/A}"
    echo "  Sensor Acelerador (max): ${ACCEL_SENSOR_MAX:-N/A}"

    if [ "${ACCEL_RAW_MAX:-0}" = "0" ] || [ -z "${ACCEL_RAW_MAX:-}" ]; then
        echo -e "  ${YELLOW}⚠ El acelerador parece quedarse en 0 en el log.${NC}"
    fi
fi

echo ""
echo "Ultimas 25 tramas de bus motores:"
grep -E "^[[:space:]]*(${IF_MOTOR_REGEX})[[:space:]]+" "${CAN_LOG}" | tail -25 || true

echo ""
echo "Log CAN guardado en: ${CAN_LOG}"
echo "Para conservarlo: cp ${CAN_LOG} /tmp/motor_e2e_$(date +%Y%m%d_%H%M%S).log"

