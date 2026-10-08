#Requires -Version 5.1
<#
    install.ps1
    ---------------------------------------------------------------
    Provisions a Windows host to build legacycore (libcci / libccic and
    the CoreConductor C# wrapper):

      1. MSYS2                    MINGW64 environment, at -MsysRoot
                                  (default C:\msys64). Downloaded if absent.
      2. MINGW64 packages         gcc, cmake, ninja (+ nodejs when no npm
                                  is on PATH - build.ps1 packs npm tarballs)
      3. protobuf 3.21.12         built from source by tools\build-protobuf.sh
                                  into .\protobuf; rebuilt whenever the gcc
                                  version changes
      4. .NET 8 SDK               CoreConductor\global.json pins 8.0.x;
                                  installed per user if absent
      5. nuget.exe                packs the .nuspec files

    Nothing here needs elevation. Idempotent: on a provisioned host it only
    verifies, and makes no network calls.

    Run once, then build:
        powershell -ExecutionPolicy Bypass -File .\install.ps1
        powershell -ExecutionPolicy Bypass -File .\build.ps1

    CI runs this too, after msys2/setup-msys2 and actions/setup-dotnet, so
    the path a developer takes is the path every pipeline takes.

    EXIT CODES
        0     toolchain ready
        1     failure (message on stderr)
#>

[CmdletBinding()]
param(
    [string]$MsysRoot = $(if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { 'C:\msys64' }),

    # Where the .NET 8 SDK goes when no 8.x SDK is found. dotnet-install's default.
    [string]$DotnetDir = (Join-Path $env:LOCALAPPDATA 'Microsoft\dotnet'),

    # Where nuget.exe goes when it is not on PATH.
    [string]$ToolsDir = 'C:\Tools',

    # Protobuf install prefix; build.ps1 reads the same default.
    [string]$ProtobufDir,

    [switch]$SkipDotnet
)

$ErrorActionPreference = 'Stop'
$ProgressPreference    = 'SilentlyContinue'

$Root = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
if (-not $ProtobufDir) { $ProtobufDir = Join-Path $Root 'protobuf' }

$script:TotalSteps = 5
$script:Step       = 0

function Write-Step([string]$Message) {
    $script:Step++
    Write-Host ""
    Write-Host "[$script:Step/$script:TotalSteps] $Message" -ForegroundColor Cyan
}
function Write-Ok   ([string]$Message) { Write-Host "      $Message" -ForegroundColor Green }
function Write-Info ([string]$Message) { Write-Host "      $Message" }

function Get-RemoteFile {
    param([Parameter(Mandatory)][string]$Url,
          [Parameter(Mandatory)][string]$Destination,
          [int]$Retries = 3)

    for ($attempt = 1; $attempt -le $Retries; $attempt++) {
        try {
            Invoke-WebRequest -Uri $Url -OutFile $Destination -UseBasicParsing
            Unblock-File -Path $Destination -ErrorAction SilentlyContinue
            return
        }
        catch {
            if ($attempt -eq $Retries) { throw "Download failed after $Retries attempts: $Url`n$($_.Exception.Message)" }
            Write-Warning "      Download attempt $attempt/$Retries failed; retrying in 5s..."
            Start-Sleep -Seconds 5
        }
    }
}

function Invoke-Msys([string]$Command, [string]$What) {
    $env:MSYSTEM        = 'MINGW64'
    $env:CHERE_INVOKING = '1'
    & (Join-Path $MsysRoot 'usr\bin\bash.exe') -lc $Command
    if ($LASTEXITCODE -ne 0) { throw "$What failed (exit $LASTEXITCODE)" }
}

# pacman -Q prints one line per installed package and an error per missing one;
# only the installed names are needed, so stderr is discarded inside bash.
function Get-MissingPackages([string[]]$Packages) {
    $env:MSYSTEM = 'MINGW64'
    $installed = @(& (Join-Path $MsysRoot 'usr\bin\bash.exe') -lc "pacman -Q $($Packages -join ' ') 2>/dev/null" |
                     ForEach-Object { ($_ -split ' ')[0] })
    @($Packages | Where-Object { $installed -notcontains $_ })
}

function Find-Dotnet8 {
    $candidates = @()
    $onPath = Get-Command dotnet -ErrorAction SilentlyContinue
    if ($onPath)          { $candidates += $onPath.Source }
    if ($env:DOTNET_ROOT) { $candidates += Join-Path $env:DOTNET_ROOT 'dotnet.exe' }
    $candidates += Join-Path $DotnetDir 'dotnet.exe'
    foreach ($dotnet in $candidates | Select-Object -Unique) {
        if (-not (Test-Path $dotnet)) { continue }
        if (@(& $dotnet --list-sdks) -match '^8\.') { return $dotnet }
    }
    $null
}

# =============================================================================
# 0. Preflight
# =============================================================================
if (-not [Environment]::Is64BitOperatingSystem) { throw "A 64-bit Windows is required." }
Write-Host "legacycore toolchain - MSYS2 at $MsysRoot"

# =============================================================================
# 1. MSYS2
# =============================================================================
Write-Step "MSYS2"
$bash = Join-Path $MsysRoot 'usr\bin\bash.exe'
if (Test-Path $bash) {
    Write-Ok "present"
}
else {
    if ((Split-Path $MsysRoot -Leaf) -ne 'msys64') {
        throw "-MsysRoot must end in \msys64 for a fresh install (the archive unpacks to msys64). Got: $MsysRoot"
    }
    $parent = Split-Path $MsysRoot -Parent
    New-Item -ItemType Directory -Force -Path $parent | Out-Null
    $sfx = Join-Path $env:TEMP 'msys2-base-x86_64-latest.sfx.exe'
    Write-Info "downloading the MSYS2 base archive"
    Get-RemoteFile 'https://github.com/msys2/msys2-installer/releases/download/nightly-x86_64/msys2-base-x86_64-latest.sfx.exe' $sfx
    Write-Info "unpacking to $parent"
    & $sfx -y "-o$parent" | Out-Null
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $bash)) { throw "MSYS2 did not unpack to $MsysRoot" }
    Remove-Item $sfx -Force

    # The first login creates the home directory and keyring. The first update
    # replaces the core runtime and ends the shell, so it gets a run of its own.
    Invoke-Msys 'true' 'MSYS2 first-run initialisation'
    Invoke-Msys 'pacman --noconfirm -Syuu' 'MSYS2 core update'
    Write-Ok "installed"
}

# =============================================================================
# 2. MINGW64 packages
# =============================================================================
Write-Step "MINGW64 toolchain packages"
$packages = @('mingw-w64-x86_64-gcc', 'mingw-w64-x86_64-cmake', 'mingw-w64-x86_64-ninja')
if (-not (Get-Command npm -ErrorAction SilentlyContinue)) {
    Write-Info "npm is not on PATH - adding MSYS2's nodejs for build.ps1's npm pack"
    $packages += 'mingw-w64-x86_64-nodejs'
}
$missing = Get-MissingPackages $packages
if ($missing.Count -eq 0) {
    Write-Ok "all present"
}
else {
    Write-Info "installing: $($missing -join ', ')"
    # -Syu, not -Sy: installing against a refreshed database without upgrading
    # is a partial upgrade, which MSYS2 does not support.
    Invoke-Msys "pacman --noconfirm --needed -Syu $($missing -join ' ')" 'pacman install'
    $missing = Get-MissingPackages $packages
    if ($missing.Count -gt 0) {
        # A core update in the same transaction can end pacman early; one more
        # pass finishes the job.
        Invoke-Msys "pacman --noconfirm --needed -Syu $($missing -join ' ')" 'pacman install (second pass)'
        $missing = Get-MissingPackages $packages
        if ($missing.Count -gt 0) { throw "Still missing after install: $($missing -join ', ')" }
    }
    Write-Ok "installed"
}

# =============================================================================
# 3. protobuf
# =============================================================================
Write-Step "protobuf (built from source, cached in $ProtobufDir)"
$env:MSYSTEM = 'MINGW64'
& (Join-Path $MsysRoot 'usr\bin\bash.exe') -l ((Join-Path $Root 'tools\build-protobuf.sh') -replace '\\', '/') `
                                              ($ProtobufDir -replace '\\', '/')
if ($LASTEXITCODE -ne 0) { throw "tools\build-protobuf.sh failed (exit $LASTEXITCODE)" }
Write-Ok (Get-Content (Join-Path $ProtobufDir '.build-stamp'))

# =============================================================================
# 4. .NET 8 SDK
# =============================================================================
Write-Step ".NET 8 SDK"
if ($SkipDotnet) {
    Write-Info "skipped (-SkipDotnet) - CoreConductor will not build"
}
else {
    $dotnet = Find-Dotnet8
    if ($dotnet) {
        Write-Ok "found: $dotnet"
    }
    else {
        $installer = Join-Path $env:TEMP 'dotnet-install.ps1'
        Get-RemoteFile 'https://dot.net/v1/dotnet-install.ps1' $installer
        & $installer -Channel 8.0 -InstallDir $DotnetDir -NoPath
        $dotnet = Find-Dotnet8
        if (-not $dotnet) { throw ".NET 8 SDK install into $DotnetDir did not produce a usable SDK" }
        Write-Ok "installed: $dotnet (build.ps1 finds it here; it is not added to PATH)"
    }
}

# =============================================================================
# 5. nuget.exe
# =============================================================================
Write-Step "nuget.exe"
$nuget = Get-Command nuget.exe -ErrorAction SilentlyContinue
if ($nuget) {
    Write-Ok "on PATH: $($nuget.Source)"
}
elseif (Test-Path (Join-Path $ToolsDir 'nuget.exe')) {
    Write-Ok "found: $(Join-Path $ToolsDir 'nuget.exe')"
}
else {
    New-Item -ItemType Directory -Force -Path $ToolsDir | Out-Null
    Get-RemoteFile 'https://dist.nuget.org/win-x86-commandline/latest/nuget.exe' (Join-Path $ToolsDir 'nuget.exe')
    Write-Ok "downloaded to $ToolsDir (build.ps1 finds it here)"
}

Write-Host ""
Write-Host "Toolchain ready. Build with: .\build.ps1" -ForegroundColor Green
exit 0
