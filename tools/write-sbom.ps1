#Requires -Version 5.1
<#
    write-sbom.ps1
    ---------------------------------------------------------------
    Writes a CycloneDX 1.5 SBOM for the files build.ps1 put in .\artifacts.

    A scanner such as syft finds components through package metadata, and
    everything that matters in these DLLs is linked statically and leaves
    none: protobuf, the GCC runtime, winpthreads, the mingw-w64 CRT. So the
    inventory is taken from the build instead:

      protobuf         version, tag and source SHA-256 from tools\build-protobuf.sh
      GCC runtime,     gcc -print-file-name names the static archives it links;
      winpthreads,     pacman -Qo names the MSYS2 package that owns each one, and
      CRT              pacman -Qi gives its version and SPDX licence
      communication    the checked-out commit of cci\packets
      artifacts        SHA-256 of every file in .\artifacts

    build.ps1 runs this after packaging. On its own:
        .\tools\write-sbom.ps1 -Version 2.3.13
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Version,
    [string]$NpmVersion,
    [string]$MsysRoot = $(if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { 'C:\msys64' }),
    [string]$ArtifactsDir,
    [string]$OutputFile
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
if (-not $NpmVersion)   { $NpmVersion   = $Version }
if (-not $ArtifactsDir) { $ArtifactsDir = Join-Path $Root 'artifacts' }
if (-not $OutputFile)   { $OutputFile   = Join-Path $Root 'sbom\bom.json' }
$gcc    = Join-Path $MsysRoot 'mingw64\bin\gcc.exe'
$pacman = Join-Path $MsysRoot 'usr\bin\pacman.exe'
foreach ($tool in $gcc, $pacman) { if (-not (Test-Path $tool)) { throw "$tool not found. Run: .\install.ps1" } }

$repoUrl  = 'https://github.com/Contact-Control-Interfaces/legacycore'
$eula     = @(@{ license = @{ name = 'Contact CI SDK EULA'; url = 'https://contact.ci/pages/sdk-eula' } })
$internal = @(@{ license = @{ name = 'Proprietary, Contact CI' } })

# Native git calls write to stderr when there is no repository (a source zip);
# Windows PowerShell 5.1 would make that terminating under 'Stop'.
function Get-GitCommit([string]$Dir) {
    $ErrorActionPreference = 'Continue'
    $sha = & git -C $Dir rev-parse HEAD 2>$null
    if ($LASTEXITCODE -eq 0) { "$sha".Trim() } else { $null }
}

function Get-Sha256([string]$Path) { (Get-FileHash $Path -Algorithm SHA256).Hash.ToLowerInvariant() }

# "spdx:GPL-3.0-or-later WITH ..." -> a CycloneDX licence expression.
function ConvertTo-Licenses([string]$Text) {
    if ($Text -match '^spdx:(.+)$') { return @(@{ expression = $Matches[1].Trim() }) }
    @(@{ license = @{ name = $Text.Trim() } })
}

# The MSYS2 packages whose static archives gcc links into a -static DLL.
function Get-RuntimePackages {
    $ErrorActionPreference = 'Continue'
    $owners = @{}
    foreach ($file in 'libstdc++.a', 'libgcc.a', 'libgcc_eh.a', 'libwinpthread.a', 'dllcrt2.o') {
        $path = (& $gcc "-print-file-name=$file").Trim()
        if (-not (Test-Path $path)) { Write-Warning "gcc does not link $file - left out of the SBOM"; continue }
        $owner = & $pacman -Qoq $path 2>$null
        if ($LASTEXITCODE -ne 0 -or -not $owner) { throw "pacman does not know which package owns $path" }
        $owners["$owner".Trim()] = $true
    }
    foreach ($name in $owners.Keys | Sort-Object) {
        $info = @{}
        foreach ($line in & $pacman -Qi $name) {
            if ($line -match '^(\w[\w ]*?)\s*:\s(.*)$') { $info[$Matches[1]] = $Matches[2].Trim() }
        }
        $purl = 'pkg:alpm/msys2/' + [Uri]::EscapeDataString($name) + "@$($info['Version'])?arch=$($info['Architecture'])"
        @{
            type      = 'library'
            'bom-ref' = $purl
            name      = $name
            version   = $info['Version']
            purl      = $purl
            licenses  = @(ConvertTo-Licenses $info['Licenses'])     # @(): a one-element array returned from a function unrolls
            externalReferences = @(@{ type = 'website'; url = $info['URL'] })
        }
    }
}

function Get-ProtobufComponent {
    $script = Get-Content (Join-Path $Root 'tools\build-protobuf.sh') -Raw
    $vars = @{}
    foreach ($key in 'PROTOBUF_VERSION', 'PROTOBUF_TAG', 'PROTOBUF_SHA256') {
        if ($script -notmatch "(?m)^$key=`"([^`"]+)`"") { throw "$key not found in tools\build-protobuf.sh" }
        $vars[$key] = $Matches[1]
    }
    $purl = "pkg:github/protocolbuffers/protobuf@$($vars.PROTOBUF_TAG)"
    @{
        type      = 'library'
        'bom-ref' = $purl
        group     = 'protocolbuffers'
        name      = 'protobuf'
        version   = $vars.PROTOBUF_VERSION
        purl      = $purl
        licenses  = @(@{ license = @{ id = 'BSD-3-Clause' } })
        externalReferences = @(@{
            type   = 'distribution'
            url    = "https://github.com/protocolbuffers/protobuf/releases/download/$($vars.PROTOBUF_TAG)/protobuf-cpp-$($vars.PROTOBUF_VERSION).tar.gz"
            hashes = @(@{ alg = 'SHA-256'; content = $vars.PROTOBUF_SHA256 })
        })
    }
}

# A file in .\artifacts, or $null when this build did not produce it
# (-SkipWrapper leaves out the CoreConductor packages).
function New-ArtifactComponent([string]$File, [string]$Type, [string]$Name, [string]$ComponentVersion,
                               [string]$Purl, $Licenses) {
    $path = Join-Path $ArtifactsDir $File
    if (-not (Test-Path $path)) { return $null }
    $component = [ordered]@{
        type      = $Type
        'bom-ref' = if ($Purl) { $Purl } else { "file:$File" }
        name      = $Name
        version   = $ComponentVersion
        hashes    = @(@{ alg = 'SHA-256'; content = Get-Sha256 $path })
        licenses  = $Licenses
        # The sbom job checks every hash against the file named here.
        properties = @(@{ name = 'legacycore:artifact'; value = $File })
    }
    if ($Purl) { $component.purl = $Purl }
    $component
}

# -----------------------------------------------------------------------------
$revision   = Get-GitCommit $Root
$packets    = Get-GitCommit (Join-Path $Root 'cci\packets')
$protobuf   = Get-ProtobufComponent
$runtime    = @(Get-RuntimePackages)
$runtimeRef = @($runtime | ForEach-Object { $_['bom-ref'] })

$communication = @{
    type      = 'data'
    'bom-ref' = 'communication'
    name      = 'communication'
    version   = if ($packets) { $packets } else { 'unknown' }
    description = 'Protocol definitions (cci/packets), generated into libcci.dll'
    licenses  = $internal
}
if ($packets) { $communication.purl = "pkg:github/Contact-Control-Interfaces/communication@$packets" }

$dllDir = Join-Path $Root 'build\release-mingw'
$dlls = foreach ($dll in 'libcci.dll', 'libccic.dll') {
    $path = Join-Path $dllDir $dll
    if (-not (Test-Path $path)) { throw "$path not found - run build.ps1 first" }
    @{
        type      = 'file'
        'bom-ref' = "file:$dll"
        name      = $dll
        version   = $Version
        hashes    = @(@{ alg = 'SHA-256'; content = Get-Sha256 $path })
        licenses  = $eula
    }
}

$npmScope = [Uri]::EscapeDataString('@contactci')
$packages = @(
    New-ArtifactComponent 'build.zip' 'file' 'build.zip' $Version $null $eula
    New-ArtifactComponent "ContactCI.Maestro.libcore.$Version.nupkg" 'library' 'ContactCI.Maestro.libcore' $Version `
        "pkg:nuget/ContactCI.Maestro.libcore@$Version" $eula
    New-ArtifactComponent "contactci-com.contactci.libcore-$NpmVersion.tgz" 'library' '@contactci/com.contactci.libcore' $NpmVersion `
        "pkg:npm/$npmScope/com.contactci.libcore@$NpmVersion" $eula
    New-ArtifactComponent "ContactCI.Maestro.CoreConductor.$Version.nupkg" 'library' 'ContactCI.Maestro.CoreConductor' $Version `
        "pkg:nuget/ContactCI.Maestro.CoreConductor@$Version" $eula
    New-ArtifactComponent "contactci-com.contactci.coreconductor-$NpmVersion.tgz" 'library' '@contactci/com.contactci.coreconductor' $NpmVersion `
        "pkg:npm/$npmScope/com.contactci.coreconductor@$NpmVersion" $eula
) | Where-Object { $_ }
if ($packages.Count -eq 0) { throw "No artifacts in $ArtifactsDir - run build.ps1 first" }

# What contains what. protobuf and the protocol definitions go into libcci
# only; the GCC runtime, winpthreads and the CRT into both DLLs.
$ref = @{}; foreach ($p in $packages) { $ref[$p.name] = $p['bom-ref'] }
$dependencies = @(
    @{ ref = 'file:libcci.dll';  dependsOn = @($protobuf['bom-ref'], 'communication') + $runtimeRef }
    @{ ref = 'file:libccic.dll'; dependsOn = @('file:libcci.dll') + $runtimeRef }
)
foreach ($p in $packages) {
    $deps = if     ($p.name -ceq 'ContactCI.Maestro.CoreConductor')        { @($ref['ContactCI.Maestro.libcore']) }
            elseif ($p.name -ceq '@contactci/com.contactci.coreconductor') { @($ref['@contactci/com.contactci.libcore']) }
            else                                                           { @('file:libcci.dll', 'file:libccic.dll') }
    $dependencies += @{ ref = $p['bom-ref']; dependsOn = @($deps | Where-Object { $_ }) }
}
$dependencies += @{ ref = 'legacycore'; dependsOn = @($packages | ForEach-Object { $_['bom-ref'] }) }

$metadataComponent = [ordered]@{
    type      = 'library'
    'bom-ref' = 'legacycore'
    name      = 'legacycore'
    version   = $Version
    licenses  = $eula
    externalReferences = @(@{ type = 'vcs'; url = $repoUrl })
}
if ($revision) { $metadataComponent.purl = "pkg:github/Contact-Control-Interfaces/legacycore@$revision" }

$bom = [ordered]@{
    bomFormat    = 'CycloneDX'
    specVersion  = '1.5'
    serialNumber = "urn:uuid:$([guid]::NewGuid())"
    version      = 1
    metadata     = [ordered]@{
        timestamp = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
        tools     = @{ components = @(@{ type = 'application'; name = 'tools/write-sbom.ps1' }) }
        component = $metadataComponent
    }
    components   = @($packages) + @($dlls) + @($protobuf, $communication) + $runtime
    dependencies = $dependencies
}

New-Item -ItemType Directory -Force -Path (Split-Path $OutputFile -Parent) | Out-Null
[IO.File]::WriteAllText($OutputFile, (ConvertTo-Json -InputObject $bom -Depth 20))
Write-Host "    SBOM: $($bom.components.Count) components -> $OutputFile" -ForegroundColor Green
