param(
    [switch]$Diag
)

$ECU_HOST = "fox@193.147.165.236"
$LOCAL_DIR = "ecu_atc8110"
$REMOTE_TAR = "fox_deploy.tar"

Write-Host "=== FOX ECU Verification Deployment ===" -ForegroundColor Cyan

# 1. Compress
Write-Host "Compressing $LOCAL_DIR..."
# Exclude large/useless dirs
tar --exclude="build" --exclude=".git" --exclude="*.exe" --exclude="*.tar" -cf $REMOTE_TAR $LOCAL_DIR

if (-not (Test-Path $REMOTE_TAR)) {
    Write-Error "Failed to create tarball."
    exit 1
}

# 2. Upload
Write-Host "Uploading to $ECU_HOST..."
scp $REMOTE_TAR "$ECU_HOST`:/home/fox/"

if ($LASTEXITCODE -ne 0) {
    Write-Error "SCP failed. Check connection."
    exit 1
}

# 3. Remote Execution
if ($Diag) {
    Write-Host "Running CAN Diagnostics on ECU..."
    $RemoteCmd = "
        echo '--- Remote: Unpacking ---'
        rm -rf /home/fox/ecu_atc8110
        tar -xf /home/fox/$REMOTE_TAR -C /home/fox/
        
        echo '--- Remote: Setting up CAN ---'
        chmod +x /home/fox/ecu_atc8110/scripts/*.sh
        # Fix CRLF issues in scripts
        sed -i 's/\r$//' /home/fox/ecu_atc8110/scripts/*.sh
        sudo /home/fox/ecu_atc8110/scripts/setup_can.sh --real
        
        echo '--- Remote: Running Diagnostic Script ---'
        sudo /home/fox/ecu_atc8110/scripts/diagnose_can.sh
    "
}
else {
    Write-Host "Building and Running ECU on Remote..."
    $RemoteCmd = "
        echo '--- Remote: Unpacking ---'
        rm -rf /home/fox/ecu_atc8110
        tar -xf /home/fox/$REMOTE_TAR -C /home/fox/
        
        echo '--- Remote: Building ---'
        mkdir -p /home/fox/ecu_atc8110/build
        cd /home/fox/ecu_atc8110/build
        cmake .. && make -j4
        
        if [ `$? -eq 0 ]; then
            echo '--- Remote: Running (Use Ctrl+C to stop) ---'
            sudo killall -q ecu_atc8110
            sleep 1
            sudo ./ecu_atc8110
        else
            echo '--- Remote: Build Failed ---'
            exit 1
        fi
    "
}


# Strip carriage returns for Linux compatibility
$RemoteCmd = $RemoteCmd -replace "`r", ""

ssh -t $ECU_HOST $RemoteCmd

# Cleanup
Remove-Item $REMOTE_TAR
Write-Host "Done."


