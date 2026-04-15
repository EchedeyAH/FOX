#!/bin/bash
#
# Diagnostico especifico del bus CAN de motores.
# Mapeo esperado:
#   - BMS:     emuccan0
#   - IMU:     emuccan1
#   - MOTORES: emuccan2
#
# Uso:
#   sudo ./scripts/diagnose_motor_can.sh [duracion_segundos]
#

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

BMS_IFACE="${BMS_IFACE:-emuccan0}"
IMU_IFACE="${IMU_IFACE:-emuccan1}"
MOTOR_IFACE="${MOTOR_IFACE:-emuccan2}"
DURATION="${1:-15}"

echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   DIAGNOSTICO CAN MOTORES (EMUCCAN2)      ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════╝${NC}"
echo ""
echo "Mapeo actual:"
echo "  BMS:     ${BMS_IFACE}"
echo "  IMU:     ${IMU_IFACE}"
echo "  MOTORES: ${MOTOR_IFACE}"
echo ""

if ! command -v candump >/dev/null 2>&1; then
    echo -e "${RED}✗ candump no disponible. Instala can-utils.${NC}"
    exit 1
fi

for iface in "${BMS_IFACE}" "${IMU_IFACE}" "${MOTOR_IFACE}"; do
    if ip link show "${iface}" >/dev/null 2>&1; then
        state="$(ip -details link show "${iface}" | grep -oP 'state \K\w+' || true)"
        echo -e "  ${GREEN}✓${NC} ${iface}: ${state:-UNKNOWN}"
    else
        echo -e "  ${RED}✗${NC} ${iface}: no existe"
        exit 1
    fi
done

echo ""
echo -e "${YELLOW}Capturando ${DURATION}s en ${MOTOR_IFACE}.${NC}"
echo "Durante la captura: pisa acelerador 2-3 veces."
echo ""

TMP_FILE="$(mktemp)"
trap 'rm -f "${TMP_FILE}"' EXIT

timeout "${DURATION}" candump "${MOTOR_IFACE}" > "${TMP_FILE}" 2>/dev/null || true

TOTAL="$(wc -l < "${TMP_FILE}" | tr -d ' ')"

count_pat() {
    local pat="$1"
    grep -Eic "${pat}" "${TMP_FILE}" || true
}

# Comandos motores (observados en el proyecto y legacy)
CMD_513="$(count_pat '(^|[[:space:]])513([[:space:]]|$)')"
CMD_514="$(count_pat '(^|[[:space:]])514([[:space:]]|$)')"
CMD_515="$(count_pat '(^|[[:space:]])515([[:space:]]|$)')"
CMD_516="$(count_pat '(^|[[:space:]])516([[:space:]]|$)')"
CMD_201="$(count_pat '(^|[[:space:]])201([[:space:]]|$)')"
CMD_202="$(count_pat '(^|[[:space:]])202([[:space:]]|$)')"
CMD_203="$(count_pat '(^|[[:space:]])203([[:space:]]|$)')"
CMD_204="$(count_pat '(^|[[:space:]])204([[:space:]]|$)')"

# Respuesta motores
RESP_281="$(count_pat '(^|[[:space:]])281([[:space:]]|$)')"
RESP_282="$(count_pat '(^|[[:space:]])282([[:space:]]|$)')"
RESP_283="$(count_pat '(^|[[:space:]])283([[:space:]]|$)')"
RESP_284="$(count_pat '(^|[[:space:]])284([[:space:]]|$)')"

echo "Resumen ${MOTOR_IFACE}:"
echo "  Tramas totales: ${TOTAL}"
echo "  CMD motores (0x513..0x516): $((CMD_513 + CMD_514 + CMD_515 + CMD_516))"
echo "  CMD legacy  (0x201..0x204): $((CMD_201 + CMD_202 + CMD_203 + CMD_204))"
echo "  RESP motores (0x281..0x284): $((RESP_281 + RESP_282 + RESP_283 + RESP_284))"
echo ""
echo "Detalle:"
echo "  0x513=${CMD_513} 0x514=${CMD_514} 0x515=${CMD_515} 0x516=${CMD_516}"
echo "  0x201=${CMD_201} 0x202=${CMD_202} 0x203=${CMD_203} 0x204=${CMD_204}"
echo "  0x281=${RESP_281} 0x282=${RESP_282} 0x283=${RESP_283} 0x284=${RESP_284}"
echo ""

if [ "${TOTAL}" -eq 0 ]; then
    echo -e "${RED}✗ Sin trafico en ${MOTOR_IFACE}.${NC}"
    echo "  Verifica emucd, cableado de motores y que ECU este ejecutandose."
    exit 2
fi

if [ $((RESP_281 + RESP_282 + RESP_283 + RESP_284)) -eq 0 ]; then
    echo -e "${YELLOW}⚠ Hay trafico pero no respuestas 0x281..0x284.${NC}"
    echo "  Posible causa: controladores no habilitados, baudrate o cableado."
else
    echo -e "${GREEN}✓ Se detectan respuestas de motores en ${MOTOR_IFACE}.${NC}"
fi

echo ""
echo "Ultimas 20 tramas capturadas:"
tail -20 "${TMP_FILE}"
