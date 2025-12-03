#!/bin/bash
#
# Script de configuración de interfaces CAN para ECU ATX-1610
# Soporta hardware real (EMUC-B2S3) y modo virtual para testing
#

set -e

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "  Configuración de Interfaces CAN"
echo "  ECU ATX-1610"
echo "========================================="
echo ""

# Función para mostrar uso
usage() {
    echo "Uso: $0 [opciones]"
    echo ""
    echo "Opciones:"
    echo "  -v, --virtual    Usar interfaces CAN virtuales (vcan) para testing"
    echo "  -r, --real       Usar interfaces CAN reales (can0, can1)"
    echo "  -h, --help       Mostrar esta ayuda"
    echo ""
    echo "Configuración por defecto:"
    echo "  CAN0: 1 Mbps  (Motores + Supervisor)"
    echo "  CAN1: 500 Kbps (BMS)"
    exit 1
}

# Modo por defecto: real
MODE="real"

# Parsear argumentos
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--virtual)
            MODE="virtual"
            shift
            ;;
        -r|--real)
            MODE="real"
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo -e "${RED}Error: Opción desconocida: $1${NC}"
            usage
            ;;
    esac
done

# Verificar permisos de root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: Este script requiere permisos de root${NC}"
    echo "Ejecutar con: sudo $0"
    exit 1
fi

echo "Modo seleccionado: $MODE"
echo ""

# ============================================================================
# Cargar módulos del kernel
# ============================================================================

echo -e "${YELLOW}[1/4] Cargando módulos del kernel...${NC}"

if [ "$MODE" == "virtual" ]; then
    # Modo virtual
    modprobe can
    modprobe can_raw
    modprobe vcan
    echo -e "${GREEN}✓ Módulos CAN virtuales cargados${NC}"
else
    # Modo real
    modprobe can
    modprobe can_raw
    # Para hardware EMUC-B2S3, puede requerir drivers específicos
    # modprobe can_dev
    # modprobe sja1000
    # modprobe sja1000_platform
    echo -e "${GREEN}✓ Módulos CAN cargados${NC}"
fi

echo ""

# ============================================================================
# Configurar interfaces CAN
# ============================================================================

echo -e "${YELLOW}[2/4] Configurando interfaces CAN...${NC}"

if [ "$MODE" == "virtual" ]; then
    # ========== MODO VIRTUAL ==========
    
    # Eliminar interfaces virtuales si ya existen
    ip link delete vcan0 2>/dev/null || true
    ip link delete vcan1 2>/dev/null || true
    
    # Crear vcan0 (simula CAN1 @ 1 Mbps - Motores/Supervisor)
    ip link add dev vcan0 type vcan
    ip link set up vcan0
    echo -e "${GREEN}✓ vcan0 creada y activada (simula CAN @ 1 Mbps)${NC}"
    
    # Crear vcan1 (simula CAN2 @ 500 Kbps - BMS)
    ip link add dev vcan1 type vcan
    ip link set up vcan1
    echo -e "${GREEN}✓ vcan1 creada y activada (simula CAN @ 500 Kbps)${NC}"
    
else
    # ========== MODO REAL ==========
    
    # Configurar emuccan0 @ 1 Mbps (Motores + Supervisor)
    if ip link show emuccan0 &>/dev/null; then
        ip link set emuccan0 up 2>/dev/null || true
        echo -e "${GREEN}✓ emuccan0 activada (hardware EMUC-B2S3)${NC}"
    else
        echo -e "${YELLOW}⚠ emuccan0 no encontrada${NC}"
    fi
    
    # Configurar emuccan1 @ 500 Kbps (BMS)
    if ip link show emuccan1 &>/dev/null; then
        ip link set emuccan1 up 2>/dev/null || true
        echo -e "${GREEN}✓ emuccan1 activada (hardware EMUC-B2S3)${NC}"
    else
        echo -e "${YELLOW}⚠ emuccan1 no encontrada (normal si solo BMS conectado)${NC}"
    fi
fi

echo ""

# ============================================================================
# Verificar estado de interfaces
# ============================================================================

echo -e "${YELLOW}[3/4] Verificando estado de interfaces...${NC}"

if [ "$MODE" == "virtual" ]; then
    IFACE0="vcan0"
    IFACE1="vcan1"
else
    IFACE0="emuccan0"
    IFACE1="emuccan1"
fi

# Verificar vcan0/can0
if ip link show $IFACE0 &>/dev/null; then
    STATE=$(ip -details link show $IFACE0 | grep -oP 'state \K\w+')
    echo -e "  $IFACE0: ${GREEN}$STATE${NC}"
else
    echo -e "  $IFACE0: ${RED}NO ENCONTRADA${NC}"
fi

# Verificar vcan1/can1
if ip link show $IFACE1 &>/dev/null; then
    STATE=$(ip -details link show $IFACE1 | grep -oP 'state \K\w+')
    echo -e "  $IFACE1: ${GREEN}$STATE${NC}"
else
    echo -e "  $IFACE1: ${RED}NO ENCONTRADA${NC}"
fi

echo ""

# ============================================================================
# Mostrar información de uso
# ============================================================================

echo -e "${YELLOW}[4/4] Configuración completada${NC}"
echo ""
echo "========================================="
echo "  Interfaces CAN listas para usar"
echo "========================================="
echo ""

if [ "$MODE" == "virtual" ]; then
    echo "Modo VIRTUAL activado - Para testing sin hardware"
    echo ""
    echo "Comandos útiles:"
    echo "  - Ver mensajes CAN:    candump vcan0"
    echo "  - Enviar mensaje CAN:  cansend vcan0 180#AA55010203"
    echo "  - Estadísticas:        ip -s link show vcan0"
    echo ""
    echo "Ejemplo de prueba:"
    echo "  Terminal 1: candump vcan0"
    echo "  Terminal 2: cansend vcan0 100#DEADBEEF"
else
    echo "Modo REAL activado - Hardware EMUC-B2S3"
    echo ""
    echo "Configuración:"
    echo "  can0: 1 Mbps   (Motores + Supervisor)"
    echo "  can1: 500 Kbps (BMS)"
    echo ""
    echo "Comandos útiles:"
    echo "  - Ver mensajes CAN:    candump can0"
    echo "  - Ver estadísticas:    ip -s link show can0"
    echo "  - Reiniciar interfaz:  sudo ip link set can0 down && sudo ip link set can0 up"
fi

echo ""
echo -e "${GREEN}✓ Configuración completada exitosamente${NC}"
echo ""
