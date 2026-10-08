#Requires -Version 5.1
<#
    build.ps1
    ---------------------------------------------------------------
    One-command build and package for legacycore. Used both locally and
    by .github/workflows/ci.yml, so a local build and a pipeline build
    produce identical outputs.

    Produces, in .\artifacts :
      build.zip                                      the bare DLLs
                                                     (build/release-mingw/*.dll)
      ContactCI.Maestro.libcore.<ver>.nupkg
      ContactCI.Maestro.CoreConductor.<ver>.nupkg
      contactci-com.contactci.libcore-<npmver>.tgz
      contactci-com.contactci.coreconductor-<npmver>.tgz
      SHA256SUMS.txt

    and .\sbom\bom.json, a CycloneDX SBOM of all of it (tools\write-sbom.ps1).

    CoreConductor's dependency on libcore is written into its packages here,
    from -Version: the same minor range for a stable version
    ([2.3.13, 2.4.0) / ~2.3.13), an exact pin for a prerelease, because NuGet
    and npm never resolve a prerelease from a plain range. The two are always
    released together, so the lower bound always exists.

    Examples:
      .\build.ps1                                   # 0.0.0-local build + package
      .\build.ps1 -Version 2.3.13
      .\build.ps1 -Version 2.4.0-beta -NpmVersion 2.4.0-beta.0
      .\build.ps1 -AllTargets                       # also the test\manual tools
      .\build.ps1 -IncludeTests                     # build and run test\unit (JUnit in reports\)
      .\build.ps1 -SkipPack                         # compile only
      .\build.ps1 -Provision                        # run install.ps1 first
#>

[CmdletBinding()]
param(
    [string]$Version = '0.0.0-local',
    [string]$NpmVersion,
    [string]$MsysRoot = $(if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { 'C:\msys64' }),
    [string]$ProtobufDir,
    [string]$OutputDir,
    [string]$ToolsDir = 'C:\Tools',
    # Also build the one-off tools under test\manual.
    [switch]$AllTargets,
    # Build and run the automated tests in test\unit; JUnit XML goes to reports\test-results.
    [switch]$IncludeTests,
    # With -IncludeTests: build the tests but do not run them (CI runs them in its `test` job).
    [switch]$SkipTestRun,
    # Skip the CoreConductor C# wrapper (and its packages).
    [switch]$SkipWrapper,
    # Compile only; produce nothing in .\artifacts.
    [switch]$SkipPack,
    # Run install.ps1 before building.
    [switch]$Provision
)

$ErrorActionPreference = 'Stop'
$ProgressPreference    = 'SilentlyContinue'

$Root     = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$BuildDir = Join-Path $Root 'build\release-mingw'      # the .nuspec and package.json point here
$PackDir  = Join-Path $Root 'build\pack'
$TestResultsDir = Join-Path $Root 'reports\test-results'
$Mingw    = Join-Path $MsysRoot 'mingw64\bin'
if (-not $ProtobufDir) { $ProtobufDir = Join-Path $Root 'protobuf' }
if (-not $OutputDir)   { $OutputDir   = Join-Path $Root 'artifacts' }
if (-not $NpmVersion)  { $NpmVersion  = $Version }
$script:step = 0

function Write-Step([string]$Message) {
    $script:step++
    Write-Host ""
    Write-Host "[$script:step] $Message" -ForegroundColor Cyan
}
function Write-Ok([string]$Message) { Write-Host "    $Message" -ForegroundColor Green }

function Invoke-Checked {
    param([string]$Exe, [string[]]$Arguments, [string]$What)
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$What failed (exit $LASTEXITCODE)" }
}

# -----------------------------------------------------------------------------
# Tool discovery. Every failure names the fix.
# -----------------------------------------------------------------------------
function Resolve-MingwTool([string]$Name) {
    $path = Join-Path $Mingw "$Name.exe"
    if (-not (Test-Path $path)) { throw "$Name.exe not found in $Mingw. Run: .\install.ps1" }
    $path
}

function Resolve-Dotnet8 {
    $candidates = @()
    $onPath = Get-Command dotnet -ErrorAction SilentlyContinue
    if ($onPath)          { $candidates += $onPath.Source }
    if ($env:DOTNET_ROOT) { $candidates += Join-Path $env:DOTNET_ROOT 'dotnet.exe' }
    $candidates += Join-Path $env:LOCALAPPDATA 'Microsoft\dotnet\dotnet.exe'
    foreach ($dotnet in $candidates | Select-Object -Unique) {
        if ((Test-Path $dotnet) -and (@(& $dotnet --list-sdks) -match '^8\.')) { return $dotnet }
    }
    throw "No .NET 8 SDK found (CoreConductor\global.json pins 8.0.x). Run: .\install.ps1, or pass -SkipWrapper"
}

function Resolve-NuGet {
    $cmd = Get-Command nuget.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $local = Join-Path $ToolsDir 'nuget.exe'
    if (Test-Path $local) { return $local }
    throw "nuget.exe not found on PATH or in $ToolsDir. Run: .\install.ps1, or pass -SkipPack"
}

function Resolve-Npm {
    $cmd = Get-Command npm.cmd -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $msys = Join-Path $Mingw 'npm.cmd'
    if (Test-Path $msys) { return $msys }
    throw "npm not found on PATH or in $Mingw. Run: .\install.ps1, or pass -SkipPack"
}

# SemVer: X.Y.Z with an optional prerelease. Mirrors tools\resolve-version.ps1.
function Split-Version([string]$Text) {
    if ($Text -notmatch '^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-([0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?$') {
        throw "'$Text' is not a valid version (X.Y.Z or X.Y.Z-suffix)"
    }
    [pscustomobject]@{ Major = [int]$Matches[1]; Minor = [int]$Matches[2]; Patch = [int]$Matches[3]; Prerelease = $Matches[4] }
}


function Copy-Staged([string]$From, [string]$To, [string[]]$Files) {
    foreach ($file in $Files) {
        $source = Join-Path $From $file
        if (-not (Test-Path $source)) { throw "Expected build output is missing: $source" }
        $target = Join-Path $To $file
        New-Item -ItemType Directory -Force -Path (Split-Path $target -Parent) | Out-Null
        Copy-Item $source $target
    }
}

function New-NuGetPackage([string]$Nuspec, [string]$Name, [scriptblock]$Edit) {
    [xml]$xml = Get-Content $Nuspec -Raw
    $ns = New-Object System.Xml.XmlNamespaceManager $xml.NameTable
    $ns.AddNamespace('n', $xml.DocumentElement.NamespaceURI)
    $metadata = $xml.SelectSingleNode('/n:package/n:metadata', $ns)
    $metadata.SelectSingleNode('n:version', $ns).InnerText = $Version

    # GitHub Packages links a NuGet package to its repository through this.
    if (-not $metadata.SelectSingleNode('n:repository', $ns)) {
        $repo = $xml.CreateElement('repository', $xml.DocumentElement.NamespaceURI)
        $repo.SetAttribute('type', 'git')
        $repo.SetAttribute('url', 'https://github.com/Contact-Control-Interfaces/legacycore')
        if ($revision) { $repo.SetAttribute('commit', $revision) }
        [void]$metadata.AppendChild($repo)
    }
    if ($Edit) { & $Edit $xml $ns }

    $generated = Join-Path $PackDir "$Name.nuspec"
    $xml.Save($generated)
    Invoke-Checked $nuget @('pack', $generated, '-BasePath', (Split-Path $Nuspec -Parent),
                           '-OutputDirectory', $OutputDir, '-NonInteractive') "nuget pack $Name"
}

# Stages package.json and its "files" into build\pack\<name> with the version
# set, then runs npm pack there. The tracked package.json is never modified.
function New-NpmPackage([string]$PackageJson, [string]$Name, [scriptblock]$Edit) {
    $stage = Join-Path $PackDir "npm-$Name"
    New-Item -ItemType Directory -Force -Path $stage | Out-Null
    $json = Get-Content $PackageJson -Raw | ConvertFrom-Json
    $json.version = $NpmVersion
    if ($Edit) { & $Edit $json }
    Copy-Staged (Split-Path $PackageJson -Parent) $stage @($json.files)
    # UTF-8 without a BOM: npm rejects a package.json that starts with one.
    [IO.File]::WriteAllText((Join-Path $stage 'package.json'), ($json | ConvertTo-Json -Depth 10))

    Push-Location $stage
    try   { Invoke-Checked $npm @('pack', '--pack-destination', $OutputDir) "npm pack $Name" }
    finally { Pop-Location }
}

# -----------------------------------------------------------------------------
# 0. Optional provisioning
# -----------------------------------------------------------------------------
if ($Provision) {
    Write-Step "Provisioning the build toolchain"
    & (Join-Path $Root 'install.ps1') -MsysRoot $MsysRoot -ProtobufDir $ProtobufDir -ToolsDir $ToolsDir -SkipDotnet:$SkipWrapper
    if ($LASTEXITCODE -ne 0) { throw "install.ps1 failed (exit $LASTEXITCODE)" }
}

# -----------------------------------------------------------------------------
# 1. Resolve version and toolchain
# -----------------------------------------------------------------------------
Write-Step "Resolving version and toolchain"
$semver   = Split-Version $Version
$revision = $null
try { $revision = (& git -C $Root rev-parse HEAD 2>$null) } catch { }    # a source zip has no .git
$cmake   = Resolve-MingwTool 'cmake'
$gcc     = Resolve-MingwTool 'gcc'
$objdump = Resolve-MingwTool 'objdump'
[void](Resolve-MingwTool 'ninja')
$dotnet  = if (-not $SkipWrapper) { Resolve-Dotnet8 }
$nuget   = if (-not $SkipPack)    { Resolve-NuGet }
$npm     = if (-not $SkipPack)    { Resolve-Npm }

# CMake, gcc and protoc find each other (and their runtime DLLs) through PATH.
$env:PATH = "$Mingw;$env:PATH"

$gccVersion = (& $gcc -dumpfullversion).Trim()
$stampFile  = Join-Path $ProtobufDir '.build-stamp'
$expected   = "MINGW64 gcc $gccVersion"
$stamp      = if (Test-Path $stampFile) { (Get-Content $stampFile -Raw).Trim() } else { '' }
if (-not $stamp.EndsWith($expected)) {
    $found = if ($stamp) { "it was built by '$stamp'" } else { 'no .build-stamp was found' }
    throw ("protobuf in $ProtobufDir does not match the current compiler (gcc $gccVersion): $found. " +
           "Run: .\install.ps1 (it rebuilds protobuf from source)")
}

Write-Ok "version   $Version (npm $NpmVersion)"
Write-Ok "revision  $(if ($revision) { $revision } else { 'unknown' })"
Write-Ok "gcc       $gccVersion"
Write-Ok "protobuf  $stamp"
if ($dotnet) { Write-Ok "dotnet    $dotnet" }

# -----------------------------------------------------------------------------
# 2. Submodule sanity - libcci is generated from cci\packets\proto
# -----------------------------------------------------------------------------
Write-Step "Checking the cci\packets submodule"
$protos = @(Get-ChildItem (Join-Path $Root 'cci\packets\proto') -Filter *.proto -ErrorAction SilentlyContinue)
if ($protos.Count -eq 0) {
    throw ("No .proto files in cci\packets\proto. They come from the communication submodule: " +
           "git submodule update --init --recursive")
}
Write-Ok "$($protos.Count) .proto file(s)"

# -----------------------------------------------------------------------------
# 3. Native build
# -----------------------------------------------------------------------------
Write-Step "Building libcci and libccic (MinGW, Release)"
# --fresh: an existing build directory left by CLion or an older toolchain
# must not leak a stale cache into a packaging build.
Invoke-Checked $cmake @('--fresh', '-G', 'Ninja', '-S', $Root, '-B', $BuildDir,
                        '-DCMAKE_BUILD_TYPE=Release', "-DCMAKE_PREFIX_PATH=$ProtobufDir",
                        "-DCCI_BUILD_UNIT_TESTS=$(if ($IncludeTests) { 'ON' } else { 'OFF' })") 'CMake configure'
$targets = if ($AllTargets -or $IncludeTests) { @() } else { @('--target', 'ccic') }
Invoke-Checked $cmake (@('--build', $BuildDir) + $targets) 'CMake build'

$dlls = @('libcci.dll', 'libccic.dll' | ForEach-Object { Join-Path $BuildDir $_ })
foreach ($dll in $dlls) {
    if (-not (Test-Path $dll)) { throw "Expected $dll was not produced" }
    $imports = @(& $objdump -p $dll | Select-String 'DLL Name:\s*(\S+)' |
                   ForEach-Object { $_.Matches[0].Groups[1].Value })
    $leaked = @($imports | Where-Object { $_ -match '^(libstdc\+\+|libgcc|libwinpthread|libprotobuf)' })
    if ($leaked.Count -gt 0) {
        throw "$(Split-Path $dll -Leaf) imports MinGW runtime DLLs ($($leaked -join ', ')) - it would not load without MSYS2 on PATH"
    }
    Write-Ok "$(Split-Path $dll -Leaf): imports $($imports -join ', ')"
}

# -----------------------------------------------------------------------------
# 3b. Automated tests - before anything is packaged
# -----------------------------------------------------------------------------
if ($IncludeTests -and $SkipTestRun) {
    Write-Step "Skipping the test run (-SkipTestRun): unit_tests.exe and api_tests.exe are in $BuildDir"
}
elseif ($IncludeTests) {
    Write-Step "Running the unit tests"
    # api_tests load the libcci.dll and libccic.dll built above - the ones that ship.
    & (Join-Path $Root 'tools\run-tests.ps1') -BinDir $BuildDir -ResultsDir $TestResultsDir
    if ($LASTEXITCODE -ne 0) { throw "Unit tests failed - see the output above. Reports: $TestResultsDir" }
}

# -----------------------------------------------------------------------------
# 4. CoreConductor
# -----------------------------------------------------------------------------
if ($SkipWrapper) {
    Write-Step "Skipping CoreConductor (-SkipWrapper)"
}
else {
    Write-Step "Building CoreConductor (Release)"
    Invoke-Checked $dotnet @('build', (Join-Path $Root 'CoreConductor\CoreConductor.sln'),
                             '-c', 'Release', '--nologo', '-p:SkipLibcorePackage=true',
                             "-p:Version=$Version", "-p:SourceRevisionId=$revision") 'dotnet build'
}

# -----------------------------------------------------------------------------
# 5. Package
# -----------------------------------------------------------------------------
if ($SkipPack) {
    Write-Step "Skipping packaging (-SkipPack)"
    exit 0
}
Write-Step "Packaging into $OutputDir"
foreach ($dir in $OutputDir, $PackDir) {
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    Get-ChildItem $dir -Force | Remove-Item -Recurse -Force
}

Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::Open((Join-Path $OutputDir 'build.zip'), 'Create')
try {
    foreach ($entry in 'build/release-mingw/libcci.dll', 'build/release-mingw/libccic.dll') {
        [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, (Join-Path $Root $entry), $entry)
    }
}
finally { $zip.Dispose() }

New-NuGetPackage (Join-Path $Root '.nuspec') 'libcore'
New-NpmPackage   (Join-Path $Root 'package.json') 'libcore'

if (-not $SkipWrapper) {
    $core = "$($semver.Major).$($semver.Minor).$($semver.Patch)"
    $nugetRange = if ($semver.Prerelease) { "[$Version]" } else { "[$core, $($semver.Major).$($semver.Minor + 1).0)" }
    $npmRange   = if ($semver.Prerelease) { $NpmVersion } else { "~$core" }
    Write-Ok "CoreConductor -> libcore: NuGet $nugetRange, npm $npmRange"

    New-NuGetPackage (Join-Path $Root 'CoreConductor\.nuspec') 'coreconductor' {
        param($xml, $ns)
        $deps = @($xml.SelectNodes("//n:dependency[@id='ContactCI.Maestro.libcore']", $ns))
        if ($deps.Count -eq 0) { throw "CoreConductor\.nuspec has no ContactCI.Maestro.libcore dependency to version" }
        $deps | ForEach-Object { $_.SetAttribute('version', $nugetRange) }
    }
    New-NpmPackage (Join-Path $Root 'CoreConductor\package.json') 'coreconductor' {
        param($json)
        $json.dependencies.'@contactci/com.contactci.libcore' = $npmRange
    }
}

$sums = Get-ChildItem $OutputDir -File | Sort-Object Name | ForEach-Object {
    "{0}  {1}" -f (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant(), $_.Name
}
[IO.File]::WriteAllLines((Join-Path $OutputDir 'SHA256SUMS.txt'), [string[]]$sums)

& (Join-Path $Root 'tools\write-sbom.ps1') -Version $Version -NpmVersion $NpmVersion -MsysRoot $MsysRoot `
                                           -ArtifactsDir $OutputDir -OutputFile (Join-Path $Root 'sbom\bom.json')

Write-Host ""
Get-ChildItem $OutputDir -File | ForEach-Object { Write-Ok ("{0,-60} {1,8:N0} KB" -f $_.Name, ($_.Length / 1KB)) }
exit 0
