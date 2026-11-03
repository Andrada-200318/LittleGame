# Run it from a PowerShell window:
#     powershell -ExecutionPolicy Bypass -File deploy.ps1
#
# Change these if a different Qt version is installed.
$qt = "C:\Qt\6.11.1\mingw_64"
$tools = "C:\Qt\Tools"

$ErrorActionPreference = "Stop"
$env:PATH = "$tools\mingw1310_64\bin;$tools\Ninja;$tools\CMake_64\bin;$qt\bin;$env:PATH"

$server = $PSScriptRoot
$build = "$server\cmake-build-release-deploy"
$dist = "$server\dist"

cmake -S $server -B $build -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_PREFIX_PATH=$qt"
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

cmake --build $build
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

if (Test-Path $dist) { Remove-Item -Recurse -Force $dist }
New-Item -ItemType Directory $dist | Out-Null
Copy-Item "$build\LittleImpostorGame.exe", "$server\questions.txt" $dist

windeployqt --release --no-translations --no-network --skip-plugin-types "generic,networkinformation,tls" "$dist\LittleImpostorGame.exe"
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

Write-Host "Done: $dist"
