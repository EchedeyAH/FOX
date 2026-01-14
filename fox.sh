#!/bin/bash
# fox.sh - Master Utility for FOX ECU Management
# Merged troubleshooting and management tools into one interface.

# ANSI Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

FOX_PASSWORD='FOX'

usage() {
    echo -e "${BLUE}FOX ECU Management Tool${NC}"
    echo "Usage: $0 {command} [args]"
    echo ""
    echo "Commands:"
    echo "  setup       Auto-detect CAN port and initialize interfaces"
    echo "  run         Recompile and run the ECU software"
    echo "  monitor     Monitor CAN traffic for motors (interactive)"
    echo "  status      Check ECU process, CAN interfaces, and traffic stats"
    echo "  diag        Deep diagnostics for EMUC hardare and system"
    echo "  test-motor  Run a continuous motor test loop (Rear Left / ID 201)"
    echo "  refresh-usb Reload USB drivers (cdc_acm) to fix stuck ports"
    echo "  restart     Restart the ECU process"
    echo ""
    exit 1
}

# ==============================================================================
# COMMAND: setup
# Logic from auto_config_can.sh
# ==============================================================================
cmd_setup() {
    echo -e "${YELLOW}=== AUTO-CONFIG CAN INTERFACE ===${NC}"
    
    echo "Stopping previous emucd_64 instances..."
    echo "$FOX_PASSWORD" | sudo -S pkill -9 emucd_64 2>/dev/null
    sleep 2

    # Find ports
    PORTS=$(ls /dev/ttyACM* 2>/dev/null)
    if [ -z "$PORTS" ]; then
        echo -e "${RED}ERROR: No /dev/ttyACM* devices found. Check USB connection.${NC}"
        return 1
    fi

    echo "Found ports: $PORTS"

    for PORT in $PORTS; do
        DEVICE_NAME=$(basename $PORT)
        echo -e "--> Trying port: ${BLUE}$PORT${NC}..."
        
        # Try to start daemon. Using -s7 (500k approx) as per successful logs.
        # Mapping: emuccan0 (Motors), emuccan1 (BMS)
        echo "$FOX_PASSWORD" | sudo -S /usr/sbin/emucd_64 -s7 -e0 $DEVICE_NAME emuccan0 emuccan1 > /tmp/emuc_auto.log 2>&1 &
        PID=$!
        sleep 3
        
        if ps -p $PID > /dev/null; then
            echo -e "${GREEN}[SUCCESS] Daemon running on $PORT (PID $PID)${NC}"
            
            echo "Configuring interfaces..."
            echo "$FOX_PASSWORD" | sudo -S ip link set emuccan0 txqueuelen 1000
            echo "$FOX_PASSWORD" | sudo -S ip link set emuccan0 up
            echo "$FOX_PASSWORD" | sudo -S ip link set emuccan1 txqueuelen 1000
            echo "$FOX_PASSWORD" | sudo -S ip link set emuccan1 up
            
            # Verify
            if ip link show emuccan0 > /dev/null 2>&1; then
                echo -e "${GREEN}[OK] emuccan0 active${NC}"
                
                if ip link show emuccan1 > /dev/null 2>&1; then
                    echo -e "${GREEN}[OK] emuccan1 active${NC}"
                else
                     echo -e "${YELLOW}[WARN] emuccan1 not active (BMS might be disconnected)${NC}"
                fi
                
                echo -e "${GREEN}Configuration SUCCESSFUL on $PORT${NC}"
                echo "$PORT" > /tmp/last_can_port
                return 0
            else
                echo -e "${RED}[FAIL] Daemon running but interfaces did not come up.${NC}"
            fi
        else
            echo -e "${RED}[FAIL] Daemon died immediately.${NC}"
            cat /tmp/emuc_auto.log
        fi
        
        # Cleanup before next try
        echo "$FOX_PASSWORD" | sudo -S pkill -9 emucd_64
        sleep 1
    done
    
    echo -e "${RED}ERROR: Could not configure CAN on any port.${NC}"
    return 1
}

# ==============================================================================
# COMMAND: run
# Logic from fix_and_run.sh
# ==============================================================================
cmd_run() {
    CLEAN=false
    if [ "$1" == "--clean" ]; then
        CLEAN=true
    fi

    echo -e "${BLUE}=== RUNNING ECU ===${NC}"
    
    # 1. Ensure CAN is up
    if ! ip link show emuccan0 >/dev/null 2>&1; then
        echo -e "${YELLOW}CAN not detected. Running setup...${NC}"
        cmd_setup
        if [ $? -ne 0 ]; then
            echo -e "${RED}Aborting run due to CAN failure.${NC}"
            exit 1
        fi
    fi

    # 2. Compilation
    echo "Building software..."
    cd ecu_atc8110
    if [ "$CLEAN" = true ]; then
        echo -e "${YELLOW}Cleaning build directory...${NC}"
        rm -rf build
    fi
    
    if [ ! -d "build" ]; then
        mkdir build
    fi
    
    cd build
    cmake ..
    if [ $? -ne 0 ]; then echo -e "${RED}CMake failed${NC}"; cd ../..; return 1; fi
    
    make -j4
    if [ $? -ne 0 ]; then echo -e "${RED}Make failed${NC}"; cd ../..; return 1; fi
    
    cd ../..
    
    # 3. Run
    echo -e "${GREEN}Starting ECU application...${NC}"
    echo "----------------------------------------------------"
    echo "HINT: To simulate pedals in another terminal:"
    echo "  echo 1 > /tmp/force_brake"
    echo "  echo 0.1 > /tmp/force_accel"
    echo "----------------------------------------------------"
    
    echo "$FOX_PASSWORD" | sudo -S ./ecu_atc8110/build/ecu_atc8110
}

# ==============================================================================
# COMMAND: monitor
# Logic from monitor_motors.sh
# ==============================================================================
cmd_monitor() {
    echo -e "${BLUE}=== MOTOR CAN TRAFFIC MONITOR ===${NC}"
    echo "Monitoring emuccan0 for IDs: 0x100 (Supervisor), 0x201-0x204 (Motors)"
    
    if ! command -v candump &> /dev/null; then
        echo -e "${RED}candump not found. Install can-utils.${NC}"
        return 1
    fi
    
    # Monitor specific IDs
    candump emuccan0,0100:07FF,0201:07FF,0202:07FF,0203:07FF,0204:07FF
}

# ==============================================================================
# COMMAND: status
# Logic from check_status.sh
# ==============================================================================
cmd_status() {
    echo -e "${BLUE}=== ECU SYSTEM STATUS ===${NC}"
    
    # Process
    if pgrep -x ecu_atc8110 > /dev/null; then
        PID=$(pgrep -x ecu_atc8110)
        echo -e "Process: ${GREEN}RUNNING${NC} (PID $PID)"
    else
        echo -e "Process: ${RED}STOPPED${NC}"
    fi
    
    # Interfaces
    echo "Interfaces:"
    for iface in emuccan0 emuccan1; do
         if ip link show $iface &>/dev/null; then
            state=$(ip link show $iface | grep -oP 'state \K\w+')
            color=$GREEN
            if [ "$state" != "UP" ]; then color=$RED; fi
            echo -e "  $iface: ${color}$state${NC}"
        else
            echo -e "  $iface: ${RED}NOT FOUND${NC}"
        fi
    done
    
    # Traffic
    echo "Traffic (last 5s):"
    count=$(timeout 5 candump emuccan0,emuccan1 2>/dev/null | wc -l)
    echo "  Messages: $count"
    
    # Logs
    echo "Latest Log:"
    tail -n 3 /home/fox/ecu_app/logs/ecu_*.log 2>/dev/null
}

# ==============================================================================
# COMMAND: diag
# Logic from diag_emuc.sh
# ==============================================================================
cmd_diag() {
    echo -e "${BLUE}=== DEEP DIAGNOSTICS ===${NC}"
    
    echo "1. Checking USB Devices (/dev/ttyACM*)"
    ls -l /dev/ttyACM*
    
    echo "2. Checking for process locks"
    lsof /dev/ttyACM0 2>/dev/null
    
    echo "3. Kernel Logs (tail)"
    dmesg | tail -10
    
    echo "4. Binary Check"
    ls -l /usr/sbin/emucd_64
    
    echo "5. Manual Test Run (Dry Run)"
    echo "$FOX_PASSWORD" | sudo -S /usr/sbin/emucd_64 -h | head -1
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Binary is executable${NC}"
    else
        echo -e "${RED}Binary execution failed${NC}"
    fi
}

# ==============================================================================
# COMMAND: test-motor
# Logic from test_motor_rear.sh
# ==============================================================================
cmd_test_motor() {
    echo -e "${YELLOW}Sending test commands to Rear Motor (ID 0x201)...${NC}"
    echo "Data: Throttle 10%, Brake 0%"
    echo "Press CTRL+C to stop"
    
    while true; do
        cansend emuccan0 201#1000
        sleep 0.05
    done
}

# ==============================================================================
# COMMAND: refresh-usb
# Logic from reload_usb.sh
# ==============================================================================
cmd_refresh_usb() {
    echo -e "${YELLOW}Reloading USB CDC_ACM drivers...${NC}"
    echo "$FOX_PASSWORD" | sudo -S modprobe -r cdc_acm
    sleep 1
    echo "$FOX_PASSWORD" | sudo -S modprobe cdc_acm
    sleep 3
    echo "Devices:"
    ls -l /dev/ttyACM*
}

# ==============================================================================
# COMMAND: restart
# Logic from restart.sh
# ==============================================================================
cmd_restart() {
    echo "Killing ECU..."
    pkill -9 ecu_atc8110 || true
    sleep 2
    
    echo "Re-running setup..."
    cmd_setup
    
    echo "Starting ECU in background..."
    cd /home/fox/ecu_app/bin/ 2>/dev/null || cd ecu_atc8110/build
    nohup sudo ./ecu_atc8110 > ../logs/ecu_$(date +%Y%m%d_%H%M%S).log 2>&1 &
    echo -e "${GREEN}Restarted (PID $!)${NC}"
}


# ==============================================================================
# MAIN DISPATCHER
# ==============================================================================

case "$1" in
    setup)
        cmd_setup
        ;;
    run)
        shift
        cmd_run "$@"
        ;;
    monitor)
        cmd_monitor
        ;;
    status)
        cmd_status
        ;;
    diag)
        cmd_diag
        ;;
    test-motor)
        cmd_test_motor
        ;;
    refresh-usb)
        cmd_refresh_usb
        ;;
    restart)
        cmd_restart
        ;;
    help|*)
        usage
        ;;
esac
