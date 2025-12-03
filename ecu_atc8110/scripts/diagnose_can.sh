#!/bin/bash
#
# Script de diagnóstico CAN para ECU ATX-1610
# Verifica que las interfaces CAN estén funcionando correctamente
#

set -e

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   DIAGNÓSTICO CAN - ECU ATX-1610      ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""

# ============================================================================
# 1. Verificar interfaces CAN disponibles
# ============================================================================

echo -e "${YELLOW}[1/5] Verificando interfaces CAN disponibles...${NC}"
echo ""

INTERFACES=$(ip link show | grep -E "emuccan|vcan|can" | awk -F: '{print $2}' | tr -d ' ')

if [ -z "$INTERFACES" ]; then
    echo -e "${RED}✗ No se encontraron interfaces CAN${NC}"
    echo -e "${YELLOW}Ejecute: sudo ./scripts/setup_can.sh${NC}"
    exit 1
fi

for iface in $INTERFACES; do
    STATE=$(ip -details link show $iface 2>/dev/null | grep -oP 'state \K\w+' || echo "UNKNOWN")
    if [ "$STATE" == "UP" ] || [ "$STATE" == "UNKNOWN" ]; then
        echo -e "  ${GREEN}✓${NC} $iface: $STATE"
    else
        echo -e "  ${RED}✗${NC} $iface: $STATE"
    fi
done

echo ""

# ============================================================================
# 2. Verificar estadísticas de CAN
# ============================================================================

echo -e "${YELLOW}[2/5] Verificando estadísticas de tráfico CAN...${NC}"
echo ""

for iface in $INTERFACES; do
    echo -e "${BLUE}Interfaz: $iface${NC}"
    
    # Obtener estadísticas
    STATS=$(ip -s link show $iface 2>/dev/null)
    
    if [ $? -eq 0 ]; then
        RX_PACKETS=$(echo "$STATS" | grep -A1 "RX:" | tail -1 | awk '{print $1}')
        TX_PACKETS=$(echo "$STATS" | grep -A1 "TX:" | tail -1 | awk '{print $1}')
        RX_ERRORS=$(echo "$STATS" | grep -A1 "RX:" | tail -1 | awk '{print $3}')
        TX_ERRORS=$(echo "$STATS" | grep -A1 "TX:" | tail -1 | awk '{print $3}')
        
        echo "  RX packets: $RX_PACKETS (errors: $RX_ERRORS)"
        echo "  TX packets: $TX_PACKETS (errors: $TX_ERRORS)"
        
        if [ "$RX_PACKETS" -gt 0 ]; then
            echo -e "  ${GREEN}✓ Recibiendo mensajes CAN${NC}"
        else
            echo -e "  ${YELLOW}⚠ No se han recibido mensajes${NC}"
        fi
        
        if [ "$TX_PACKETS" -gt 0 ]; then
            echo -e "  ${GREEN}✓ Enviando mensajes CAN${NC}"
        else
            echo -e "  ${YELLOW}⚠ No se han enviado mensajes${NC}"
        fi
    fi
    echo ""
done

# ============================================================================
# 3. Capturar tráfico CAN en tiempo real (5 segundos)
# ============================================================================

echo -e "${YELLOW}[3/5] Capturando tráfico CAN (5 segundos)...${NC}"
echo ""

# Seleccionar primera interfaz disponible
FIRST_IFACE=$(echo $INTERFACES | awk '{print $1}')

if command -v candump &> /dev/null; then
    echo -e "${BLUE}Monitoreando $FIRST_IFACE...${NC}"
    echo ""
    
    # Capturar durante 5 segundos
    timeout 5 candump $FIRST_IFACE 2>/dev/null | head -20 || true
    
    echo ""
    echo -e "${GREEN}✓ Captura completada${NC}"
else
    echo -e "${YELLOW}⚠ candump no disponible (instalar can-utils)${NC}"
fi

echo ""

# ============================================================================
# 4. Verificar IDs CAN esperados
# ============================================================================

echo -e "${YELLOW}[4/5] Verificando IDs CAN esperados...${NC}"
echo ""

echo "IDs CAN del sistema FOX:"
echo "  BMS:        0x180 (384)"
echo "  Motor 1:    0x201 (513) CMD / 0x281 (641) RESP"
echo "  Motor 2:    0x202 (514) CMD / 0x282 (642) RESP"
echo "  Motor 3:    0x203 (515) CMD / 0x283 (643) RESP"
echo "  Motor 4:    0x204 (516) CMD / 0x284 (644) RESP"
echo "  Supervisor: 0x100 (256) HB  / 0x101 (257) CMD"
echo ""

# Capturar 10 segundos y contar IDs
if command -v candump &> /dev/null; then
    echo -e "${BLUE}Analizando tráfico por 10 segundos...${NC}"
    
    TEMP_FILE=$(mktemp)
    timeout 10 candump $FIRST_IFACE 2>/dev/null > $TEMP_FILE || true
    
    # Contar mensajes por ID
    echo ""
    echo "Mensajes recibidos por ID:"
    
    # BMS
    BMS_COUNT=$(grep -c "180" $TEMP_FILE || echo "0")
    if [ "$BMS_COUNT" -gt 0 ]; then
        echo -e "  ${GREEN}✓${NC} BMS (0x180): $BMS_COUNT mensajes"
    else
        echo -e "  ${RED}✗${NC} BMS (0x180): No detectado"
    fi
    
    # Motores
    for i in 1 2 3 4; do
        CMD_ID=$(printf "%X" $((0x200 + i)))
        RESP_ID=$(printf "%X" $((0x280 + i)))
        
        CMD_COUNT=$(grep -ci "$CMD_ID" $TEMP_FILE || echo "0")
        RESP_COUNT=$(grep -ci "$RESP_ID" $TEMP_FILE || echo "0")
        
        if [ "$CMD_COUNT" -gt 0 ] || [ "$RESP_COUNT" -gt 0 ]; then
            echo -e "  ${GREEN}✓${NC} Motor $i: CMD=$CMD_COUNT, RESP=$RESP_COUNT"
        else
            echo -e "  ${YELLOW}⚠${NC} Motor $i: No detectado"
        fi
    done
    
    # Supervisor
    HB_COUNT=$(grep -c "100" $TEMP_FILE || echo "0")
    CMD_COUNT=$(grep -c "101" $TEMP_FILE || echo "0")
    
    if [ "$HB_COUNT" -gt 0 ] || [ "$CMD_COUNT" -gt 0 ]; then
        echo -e "  ${GREEN}✓${NC} Supervisor: HB=$HB_COUNT, CMD=$CMD_COUNT"
    else
        echo -e "  ${YELLOW}⚠${NC} Supervisor: No detectado"
    fi
    
    rm -f $TEMP_FILE
fi

echo ""

# ============================================================================
# 5. Resumen y recomendaciones
# ============================================================================

echo -e "${YELLOW}[5/5] Resumen y recomendaciones${NC}"
echo ""

echo -e "${BLUE}Estado del sistema CAN:${NC}"

# Verificar si hay tráfico
HAS_TRAFFIC=false
for iface in $INTERFACES; do
    RX=$(ip -s link show $iface 2>/dev/null | grep -A1 "RX:" | tail -1 | awk '{print $1}')
    if [ "$RX" -gt 0 ]; then
        HAS_TRAFFIC=true
        break
    fi
done

if $HAS_TRAFFIC; then
    echo -e "  ${GREEN}✓ Hay tráfico CAN activo${NC}"
else
    echo -e "  ${RED}✗ No se detecta tráfico CAN${NC}"
    echo ""
    echo -e "${YELLOW}Recomendaciones:${NC}"
    echo "  1. Verificar conexiones físicas CAN"
    echo "  2. Verificar que los dispositivos estén encendidos"
    echo "  3. Ejecutar: sudo ./scripts/setup_can.sh"
    echo "  4. Verificar terminación CAN (120Ω)"
fi

echo ""
echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   DIAGNÓSTICO COMPLETADO              ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""

echo "Comandos útiles:"
echo "  - Monitorear CAN:     candump $FIRST_IFACE"
echo "  - Ver estadísticas:   ip -s link show $FIRST_IFACE"
echo "  - Reiniciar interfaz: sudo ip link set $FIRST_IFACE down && sudo ip link set $FIRST_IFACE up"
echo ""
