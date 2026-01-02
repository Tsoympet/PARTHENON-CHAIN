$ErrorActionPreference = "Stop"

$buildDir = "build"
if (!(Test-Path $buildDir)) { New-Item -ItemType Directory -Path $buildDir | Out-Null }

cmake -S . -B $buildDir -DCMAKE_BUILD_TYPE=Release
cmake --build $buildDir --target drachmad drachma_cli

$dest = "${Env:ProgramFiles}\Drachma"
New-Item -ItemType Directory -Force -Path $dest | Out-Null
Copy-Item "$buildDir\layer1-core\drachmad.exe" "$dest\drachmad.exe"
Copy-Item "$buildDir\layer1-core\drachma-cli.exe" "$dest\drachma-cli.exe"
Write-Output "Installed binaries to $dest"
