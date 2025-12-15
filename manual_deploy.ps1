$ECU_IP = "193.147.165.236"
$USER = "fox"
$PROJECT_DIR = "c:\Users\ahech\Desktop\FOX\ecu_atc8110"
$REMOTE_DIR = "/home/fox/ecu_atc8110"

Write-Host "=== FOX ECU DEPLOYMENT SCRIPT ===" -ForegroundColor Cyan
Write-Host "Preparing to deploy to $USER@$ECU_IP..."

# 1. Compress Project
Write-Host "1. Compressing project files..."
$tarFile = "c:\Users\ahech\Desktop\FOX\fox_ecu_deploy.tar"
tar --exclude="build" --exclude=".git" --exclude="*.exe" --exclude="*.tar" -cvf $tarFile -C "c:\Users\ahech\Desktop\FOX" "ecu_atc8110"

if (-not (Test-Path $tarFile)) {
    Write-Error "Failed to create archive."
    exit 1
}

# 2. Upload
Write-Host "2. Uploading code to ECU (Prepare to enter password: FOX)..."
scp $tarFile ${USER}@${ECU_IP}:/home/fox/

# 3. Build & Run
Write-Host "3. Connecting to ECU to Build and Run..."

# Construct command line with minimal newlines to avoid CRLF issues
$cmd_unpack = "rm -rf $REMOTE_DIR; tar -xvf fox_ecu_deploy.tar; cd ecu_atc8110"
$cmd_build = "mkdir -p build; cd build; cmake ..; make -j4"
# Run with interaction
$cmd_run = "echo '--- Running ---'; sudo killall -q ecu_atc8110; sleep 1; echo 'PRESS BRAKE PEDAL TO START SYSTEM'; sudo ./ecu_atc8110"

# Combine
$remote_cmds = "$cmd_unpack && $cmd_build && $cmd_run"

# Execute
ssh -t ${USER}@${ECU_IP} "$remote_cmds"

Write-Host "Deployment finished."
