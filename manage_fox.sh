#!/bin/bash
# manage_fox.sh - Master Management Script for FOX ECU (Host Side)
# Use this script from your PC (Linux/WSL) to deploy code and manage the ECU.

# Configuration
ECU_IP="193.147.165.236"
ECU_USER="fox"
ECU_HOST="${ECU_USER}@${ECU_IP}"
LOCAL_DIR="ecu_atc8110"
REMOTE_WORK_DIR="/home/fox/ecu_atc8110"

# ANSI Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

usage() {
    echo -e "${BLUE}FOX ECU Host Manager${NC}"
    echo "Usage: $0 {command}"
    echo ""
    echo "Commands:"
    echo "  deploy          Full deploy: Check filesystem, tar, scp, build, and run."
    echo "  deploy-fast     Fast deploy: Sync only changed .cpp/.hpp/CMake files."
    echo "  diag            Deploy and run sensor diagnostics."
    echo "  fix-fs          Run filesystem fix (replace std::filesystem with fs::)."
    echo "  ssh             SSH into the ECU."
    echo ""
    exit 1
}

check_ssh_key() {
    # Check if we can connect without password, imply user might need to set up keys if hangs
    echo -e "${YELLOW}Testing SSH connection to $ECU_HOST...${NC}"
    if ! ssh -o BatchMode=yes -o ConnectTimeout=5 "$ECU_HOST" "echo 'SSH OK'" 2>/dev/null; then
        echo -e "${YELLOW}[WARN] Passwordless SSH not configured.${NC}"
        echo -e "${YELLOW}       You may be prompted for the password (default: FOX) multiple times.${NC}"
        echo -e "${YELLOW}       Pro-tip: Run 'ssh-copy-id $ECU_HOST' to allow automatic login.${NC}"
        echo ""
    else
        echo -e "${GREEN}[OK] SSH connection established.${NC}"
    fi
}

cmd_fix_fs() {
    if [ -f "./fix_filesystem.sh" ]; then
        bash ./fix_filesystem.sh
    else
        echo -e "${RED}Error: fix_filesystem.sh not found.${NC}"
    fi
}

cmd_deploy() {
    echo -e "${BLUE}=== FULL DEPLOYMENT ===${NC}"
    
    # 1. Fix Filesystem
    cmd_fix_fs
    
    # 2. Compress
    echo "Compressing..."
    tar --exclude="build" --exclude=".git" --exclude="*.exe" --exclude="*.tar" -cf fox_deploy.tar "$LOCAL_DIR"
    
    # 3. Upload
    echo "Uploading to $ECU_HOST..."
    scp fox_deploy.tar "$ECU_HOST:/home/fox/"
    
    # 4. Remote Build & Run
    echo "Building and Running on ECU..."
    echo -e "${YELLOW}NOTE: Press Ctrl+C to stop the remote process.${NC}"
    
    ssh -t "$ECU_HOST" "
        echo '--- Remote: Unpacking ---'
        rm -rf $REMOTE_WORK_DIR
        tar -xf fox_deploy.tar
        
        echo '--- Remote: Building ---'
        mkdir -p $REMOTE_WORK_DIR/build
        cd $REMOTE_WORK_DIR/build
        cmake .. && make -j4
        
        if [ \$? -eq 0 ]; then
            echo '--- Remote: Running ---'
            echo 'PRESS BRAKE PEDAL TO START'
            sudo killall -q ecu_atc8110
            sleep 1
            sudo ./ecu_atc8110
        else
            echo '--- Remote: Build Failed ---'
        fi
    "
    
    # Cleanup
    rm fox_deploy.tar
}

cmd_deploy_fast() {
    echo -e "${BLUE}=== FAST DEPLOY (Sync Changes) ===${NC}"
    
    # 1. Fix Filesystem
    cmd_fix_fs
    
    # 2. Sync Files (using rsync if available, else tar subset? scp is safer here for raw files)
    # Since Windows might not have rsync, we use scp for specific folders or just scp recursive
    # But scp -r is slow. Let's send a tar of code only.
    
    echo "Compressing source files..."
    tar -cf fox_code.tar "$LOCAL_DIR"/*.cpp "$LOCAL_DIR"/*.hpp "$LOCAL_DIR"/*.txt "$LOCAL_DIR"/*/*.cpp "$LOCAL_DIR"/*/*.hpp
    
    echo "Uploading..."
    scp fox_code.tar "$ECU_HOST:/home/fox/"
    
    echo "Patching and Building..."
    ssh -t "$ECU_HOST" "
        cd /home/fox
        tar -xf fox_code.tar
        rm fox_code.tar
        
        cd $REMOTE_WORK_DIR/build
        make -j4
        
        if [ \$? -eq 0 ]; then
            echo '--- Remote: Running ---'
            sudo killall -q ecu_atc8110
            sleep 1
            sudo ./ecu_atc8110
        fi
    "
    rm fox_code.tar
}

cmd_diag() {
    echo -e "${BLUE}=== SENSOR DIAGNOSTIC ===${NC}"
    
    echo "Uploading sensor_diagnostic.cpp..."
    scp "$LOCAL_DIR/tools/sensor_diagnostic.cpp" "$ECU_HOST:$REMOTE_WORK_DIR/tools/"
    
    echo "Running Diagnostic on ECU..."
    ssh -t "$ECU_HOST" "
        cd $REMOTE_WORK_DIR/build
        make sensor_diagnostic
        if [ \$? -eq 0 ]; then
            echo '--- Starting Diagnostic ---'
            sudo ./tools/sensor_diagnostic
        else
            echo 'Build failed.'
        fi
    "
}

# Main
check_ssh_key

case "$1" in
    deploy)
        cmd_deploy
        ;;
    deploy-fast)
        cmd_deploy_fast
        ;;
    diag)
        cmd_diag
        ;;
    fix-fs)
        cmd_fix_fs
        ;;
    ssh)
        ssh "$ECU_HOST"
        ;;
    help|*)
        usage
        ;;
esac
