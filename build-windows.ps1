param(
    [string]$Configuration = "RelWithDebInfo"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$TemplateDir = Join-Path $Root ".obs-template-build"
$DistDir = Join-Path $Root "dist"

function Require-Command([string]$Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command '$Name' was not found. Install it and run this script again."
    }
}

Require-Command git
Require-Command cmake

if (Test-Path $TemplateDir) {
    Remove-Item $TemplateDir -Recurse -Force
}

Write-Host "Cloning the official OBS plugin template..."
git clone --depth 1 https://github.com/obsproject/obs-plugintemplate.git $TemplateDir

Write-Host "Overlaying OBS Explorer Dock sources..."
Remove-Item (Join-Path $TemplateDir "src\*") -Recurse -Force
Copy-Item (Join-Path $Root "src\*") (Join-Path $TemplateDir "src") -Recurse -Force
Remove-Item (Join-Path $TemplateDir "data\*") -Recurse -Force
Copy-Item (Join-Path $Root "data\*") (Join-Path $TemplateDir "data") -Recurse -Force
Copy-Item (Join-Path $Root "CMakeLists.txt") (Join-Path $TemplateDir "CMakeLists.txt") -Force
Copy-Item (Join-Path $Root "buildspec.json") (Join-Path $TemplateDir "buildspec.json") -Force

Push-Location $TemplateDir
try {
    Write-Host "Configuring..."
    cmake --preset windows-x64 -DENABLE_FRONTEND_API=ON -DENABLE_QT=ON

    Write-Host "Building..."
    cmake --build --preset windows-x64 --config $Configuration
} finally {
    Pop-Location
}

if (Test-Path $DistDir) {
    Remove-Item $DistDir -Recurse -Force
}
New-Item -ItemType Directory -Path $DistDir | Out-Null

$dll = Get-ChildItem $TemplateDir -Recurse -Filter "obs-explorer-dock.dll" | Where-Object { $_.FullName -match $Configuration } | Select-Object -First 1
$pdb = Get-ChildItem $TemplateDir -Recurse -Filter "obs-explorer-dock.pdb" | Where-Object { $_.FullName -match $Configuration } | Select-Object -First 1

if (-not $dll) {
    throw "Build completed but obs-explorer-dock.dll was not found."
}

Copy-Item $dll.FullName (Join-Path $DistDir "obs-explorer-dock.dll") -Force
if ($pdb) {
    Copy-Item $pdb.FullName (Join-Path $DistDir "obs-explorer-dock.pdb") -Force
}
Copy-Item (Join-Path $Root "data") (Join-Path $DistDir "data") -Recurse -Force

Write-Host ""
Write-Host "Build finished. Files are in: $DistDir" -ForegroundColor Green
Write-Host "Install the DLL to: C:\Program Files\obs-studio\obs-plugins\64bit\"
Write-Host "Install locale files to: C:\Program Files\obs-studio\data\obs-plugins\obs-explorer-dock\locale\"
