param(
    [ValidateSet("OBS32", "OBS33")]
    [string]$ObsTarget = "OBS32",
    [string]$Configuration = "RelWithDebInfo"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$TemplateDir = Join-Path $Root ".obs-template-build-$ObsTarget"
$DistDir = Join-Path $Root "dist/$ObsTarget"

$BuildSpecPath = if ($ObsTarget -eq "OBS33") {
    Join-Path $Root "buildspec-obs33.json"
} else {
    Join-Path $Root "buildspec.json"
}

function Require-Command([string]$Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command '$Name' was not found. Install it and run this script again."
    }
}

Require-Command git
Require-Command cmake

if (-not (Test-Path $BuildSpecPath)) {
    throw "Build specification not found: $BuildSpecPath"
}

if (Test-Path $TemplateDir) {
    Remove-Item $TemplateDir -Recurse -Force
}

Write-Host "Cloning the official OBS plugin template for $ObsTarget..."
git clone --depth 1 https://github.com/obsproject/obs-plugintemplate.git $TemplateDir
if ($LASTEXITCODE -ne 0) {
    throw "Could not clone the official OBS plugin template."
}

Write-Host "Overlaying Explorer Dock sources and $ObsTarget build specification..."
Remove-Item (Join-Path $TemplateDir "src\*") -Recurse -Force
Copy-Item (Join-Path $Root "src\*") (Join-Path $TemplateDir "src") -Recurse -Force
Remove-Item (Join-Path $TemplateDir "data\*") -Recurse -Force
Copy-Item (Join-Path $Root "data\*") (Join-Path $TemplateDir "data") -Recurse -Force
Copy-Item (Join-Path $Root "CMakeLists.txt") (Join-Path $TemplateDir "CMakeLists.txt") -Force
Copy-Item $BuildSpecPath (Join-Path $TemplateDir "buildspec.json") -Force

if ($ObsTarget -eq "OBS33") {
    # OBS 33 publishes its source archive as a versioned CPack tarball rather than
    # the GitHub tag ZIP used by the OBS 32 build specification.
    $windowsBuildSpec = Join-Path $TemplateDir "cmake\windows\buildspec.cmake"
    $windowsBuildSpecContent = Get-Content $windowsBuildSpec -Raw
    $oldArchiveName = 'set(obs-studio_filename "VERSION.zip")'
    $newArchiveName = 'set(obs-studio_filename "OBS-Studio-VERSION-Sources.tar.gz")'
    $oldSourceDirectory = 'set(obs-studio_destination "obs-studio-VERSION")'
    $newSourceDirectory = 'set(obs-studio_destination "obs-studio-VERSION-sources")'

    if (-not $windowsBuildSpecContent.Contains($oldArchiveName)) {
        throw "Could not locate the OBS source archive filename in $windowsBuildSpec."
    }
    if (-not $windowsBuildSpecContent.Contains($oldSourceDirectory)) {
        throw "Could not locate the OBS source directory in $windowsBuildSpec."
    }

    $windowsBuildSpecContent = $windowsBuildSpecContent.Replace($oldArchiveName, $newArchiveName)
    $windowsBuildSpecContent = $windowsBuildSpecContent.Replace($oldSourceDirectory, $newSourceDirectory)
    Set-Content -Path $windowsBuildSpec -Value $windowsBuildSpecContent -Encoding utf8NoBOM
}

Push-Location $TemplateDir
try {
    Write-Host "Configuring $ObsTarget..."
    cmake --preset windows-x64 -DENABLE_FRONTEND_API=ON -DENABLE_QT=ON
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed for $ObsTarget."
    }

    Write-Host "Building $ObsTarget..."
    cmake --build --preset windows-x64 --config $Configuration
    if ($LASTEXITCODE -ne 0) {
        throw "CMake build failed for $ObsTarget."
    }
} finally {
    Pop-Location
}

if (Test-Path $DistDir) {
    Remove-Item $DistDir -Recurse -Force
}
New-Item -ItemType Directory -Path $DistDir -Force | Out-Null

$dll = Get-ChildItem $TemplateDir -Recurse -Filter "obs-explorer-dock.dll" | Where-Object { $_.FullName -match $Configuration } | Select-Object -First 1
$pdb = Get-ChildItem $TemplateDir -Recurse -Filter "obs-explorer-dock.pdb" | Where-Object { $_.FullName -match $Configuration } | Select-Object -First 1

if (-not $dll) {
    throw "$ObsTarget build completed but obs-explorer-dock.dll was not found."
}

Copy-Item $dll.FullName (Join-Path $DistDir "obs-explorer-dock.dll") -Force
if ($pdb) {
    Copy-Item $pdb.FullName (Join-Path $DistDir "obs-explorer-dock.pdb") -Force
}
Copy-Item (Join-Path $Root "data") (Join-Path $DistDir "data") -Recurse -Force

Write-Host ""
Write-Host "$ObsTarget build finished. Files are in: $DistDir" -ForegroundColor Green
if ($ObsTarget -eq "OBS33") {
    Write-Host "Install the plugin folder to: %ProgramData%\obs-studio\plugins\obs-explorer-dock\"
} else {
    Write-Host "Install the DLL to: C:\Program Files\obs-studio\obs-plugins\64bit\"
    Write-Host "Install locale files to: C:\Program Files\obs-studio\data\obs-plugins\obs-explorer-dock\locale\"
}
