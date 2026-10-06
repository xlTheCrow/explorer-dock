param(
    [string]$ObsPath = "C:\Program Files\obs-studio"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Dist = Join-Path $Root "dist"

if (-not (Test-Path (Join-Path $Dist "obs-explorer-dock.dll"))) {
    throw "No built DLL found. Run build-windows.ps1 first."
}

$PluginDir = Join-Path $ObsPath "obs-plugins\64bit"
$DataDir = Join-Path $ObsPath "data\obs-plugins\obs-explorer-dock\locale"

New-Item -ItemType Directory -Force -Path $PluginDir | Out-Null
New-Item -ItemType Directory -Force -Path $DataDir | Out-Null

Copy-Item (Join-Path $Dist "obs-explorer-dock.dll") $PluginDir -Force
if (Test-Path (Join-Path $Dist "obs-explorer-dock.pdb")) {
    Copy-Item (Join-Path $Dist "obs-explorer-dock.pdb") $PluginDir -Force
}
Copy-Item (Join-Path $Root "data\locale\*.ini") $DataDir -Force

Write-Host "OBS Explorer Dock installed. Restart OBS Studio." -ForegroundColor Green
