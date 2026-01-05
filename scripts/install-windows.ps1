# DRACHMA Windows Installation Script
# Mirrors Bitcoin Core's installation flow
$ErrorActionPreference = "Stop"

Write-Host "=== DRACHMA Blockchain Installation (Windows) ===" -ForegroundColor Green
Write-Host ""

$buildDir = "build"
$installPrefix = "${Env:ProgramFiles}\Drachma"
$buildType = "Release"

# Check for CMake
try {
    $cmakeVersion = cmake --version 2>$null
    Write-Host "CMake found: $($cmakeVersion[0])" -ForegroundColor Green
} catch {
    Write-Host "ERROR: CMake not found. Please install CMake and try again." -ForegroundColor Red
    exit 1
}

Write-Host "Build directory: $buildDir"
Write-Host "Install prefix: $installPrefix"
Write-Host "Build type: $buildType"
Write-Host ""

# Create build directory if it doesn't exist
if (!(Test-Path $buildDir)) { 
    New-Item -ItemType Directory -Path $buildDir | Out-Null 
}

# Configure with CMake
Write-Host "Configuring build..." -ForegroundColor Yellow
cmake -S . -B $buildDir -DCMAKE_BUILD_TYPE=$buildType -DCMAKE_INSTALL_PREFIX="$installPrefix"

# Build
Write-Host "Building DRACHMA binaries..." -ForegroundColor Yellow
cmake --build $buildDir --config $buildType --parallel

# Install
Write-Host "Installing DRACHMA to $installPrefix..." -ForegroundColor Yellow
cmake --build $buildDir --config $buildType --target install

Write-Host ""
Write-Host "=== Installation Complete ===" -ForegroundColor Green
Write-Host ""
Write-Host "Installed binaries to: $installPrefix\bin"
Write-Host "  - drachmad.exe      : DRACHMA daemon"
Write-Host "  - drachma-cli.exe   : RPC client"
Write-Host "  - drachma-cpuminer.exe : CPU miner"
if (Test-Path "$installPrefix\bin\drachma-qt.exe") {
    Write-Host "  - drachma-qt.exe    : Desktop wallet"
}
Write-Host ""
Write-Host "Documentation installed to: $installPrefix\share\doc\drachma"
Write-Host ""
Write-Host "Add $installPrefix\bin to your PATH to use the binaries from any location."
Write-Host ""
Write-Host "To start the daemon:"
Write-Host "  $installPrefix\bin\drachmad.exe --network=testnet"
Write-Host ""
Write-Host "For help:"
Write-Host "  $installPrefix\bin\drachmad.exe --help"

