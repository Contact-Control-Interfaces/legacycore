#Requires -Version 5.1
<#
    resolve-version.ps1
    ---------------------------------------------------------------
    Decides the package version for a CI run from the git ref, and prints
    it as key=value lines for $GITHUB_OUTPUT. Kept out of the workflow so
    the rules can be run and checked locally:

        .\tools\resolve-version.ps1 -Ref refs/tags/v3.0.3-rc.2
        .\tools\resolve-version.ps1 -Ref refs/heads/staging -RunNumber 42

    The rules are the dev-handbook's (releases/versioning.md):

      refs/tags/vX.Y.Z        kind=release, a release. Must be on main.
      refs/tags/vX.Y.Z-rc.N   kind=release, a release candidate (GitHub
                              prerelease, npm dist-tag next). Must be on staging.
      refs/tags/<other>       throws, so a mistyped tag fails before anything
                              is built or published.
      refs/heads/staging      kind=beta, version <declared>-beta.<run number>,
                              published to the internal NuGet feed only. ("beta",
                              not "staging": SemVer sorts prerelease labels
                              alphabetically, and "staging" would outrank "rc".)
      anything else           kind=ci, version <declared>-ci.<run number>,
                              workflow artifacts only.

    <declared> is X.Y.Z from the VERSION file at the repo root: the version
    staging is working towards. A tag's X.Y.Z must equal it, and it must be
    above the highest release tag, so bump VERSION on staging right after a
    release. The versions in .nuspec and package.json are placeholders that
    build.ps1 overwrites at pack time.

    The branch a tag must be on is checked by the workflow's release job, not
    here: it depends on the commit being built, which a local run would not
    reproduce.

    Outputs: version, npm_version, prerelease, npm_tag, kind, required_branch
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Ref,
    [int]$RunNumber = 0,
    [string]$RepoDir
)

$ErrorActionPreference = 'Stop'

if (-not $RepoDir) { $RepoDir = Split-Path -Parent $PSScriptRoot }

# Semver core with an optional dotted prerelease. No build metadata: NuGet
# drops it and npm strips it, so two tags could collide on one package version.
$SemVerPattern = '^v?(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-([0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?$'
# What may be tagged from now on: a release or a numbered release candidate, with the v prefix.
$ReleaseTagPattern = '^v(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(-rc\.[1-9]\d*)?$'

function ConvertTo-SemVer([string]$Text) {
    if ($Text -notmatch $SemVerPattern) { return $null }
    [pscustomobject]@{
        Text       = $Text -replace '^v', ''
        Core       = "$($Matches[1]).$($Matches[2]).$($Matches[3])"
        Major      = [int]$Matches[1]
        Minor      = [int]$Matches[2]
        Patch      = [int]$Matches[3]
        Prerelease = $Matches[4]
    }
}

# SemVer 2.0 precedence. Returns <0, 0 or >0.
function Compare-SemVer($A, $B) {
    foreach ($part in 'Major', 'Minor', 'Patch') {
        if ($A.$part -ne $B.$part) { return $A.$part - $B.$part }
    }
    if (-not $A.Prerelease -and -not $B.Prerelease) { return 0 }
    if (-not $A.Prerelease) { return 1 }       # 1.0.0 > 1.0.0-anything
    if (-not $B.Prerelease) { return -1 }
    $ai = $A.Prerelease.Split('.'); $bi = $B.Prerelease.Split('.')
    for ($i = 0; $i -lt [Math]::Min($ai.Count, $bi.Count); $i++) {
        $an = $ai[$i] -match '^\d+$'; $bn = $bi[$i] -match '^\d+$'
        if ($an -and $bn) { $c = [long]$ai[$i] - [long]$bi[$i] }
        elseif ($an)      { $c = -1 }              # numeric < alphanumeric
        elseif ($bn)      { $c = 1 }
        else              { $c = [string]::CompareOrdinal($ai[$i], $bi[$i]) }
        if ($c -ne 0) { return $c }
    }
    $ai.Count - $bi.Count
}

function Get-HighestTag {
    $tags = @(& git -C $RepoDir tag --list)
    if ($LASTEXITCODE -ne 0) { throw "git tag --list failed in $RepoDir" }
    $highest = $null
    foreach ($tag in $tags) {
        $v = ConvertTo-SemVer $tag
        if ($v -and (-not $highest -or (Compare-SemVer $v $highest) -gt 0)) { $highest = $v }
    }
    $highest
}

# The GitLab pipeline published 2.1.15-beta to npm as 2.1.15-beta.0; keep
# doing that so npm versions stay continuous with what is already published.
function Get-NpmVersion($V) {
    if (-not $V.Prerelease -or $V.Prerelease -match '(^|\.)\d+$') { return $V.Text }
    "$($V.Text).0"
}

# The version the repo is working towards (dev-handbook versioning rule 3).
function Get-DeclaredVersion {
    $path = Join-Path $RepoDir 'VERSION'
    if (-not (Test-Path $path)) { throw "VERSION not found at $path. It holds the X.Y.Z the repository is working towards." }
    $text = (Get-Content $path -Raw).Trim()
    if ($text -notmatch '^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$') { throw "VERSION must be X.Y.Z with no prefix or suffix (got '$text')." }
    ConvertTo-SemVer $text
}

$declared = Get-DeclaredVersion
$highest  = Get-HighestTag
# Only releases count here: an rc of the declared version is expected to exist.
if ($highest -and -not $highest.Prerelease -and (Compare-SemVer $declared $highest) -le 0) {
    throw ("VERSION says $($declared.Text) but $($highest.Text) is already released. " +
           "Bump VERSION on staging to the next version.")
}

$requiredBranch = ''
if ($Ref -match '^refs/tags/(.+)$') {
    $tag = $Matches[1]
    if ($tag -notmatch $ReleaseTagPattern) {
        throw ("Tag '$tag' is not a release tag. Tag vX.Y.Z on main for a release, or " +
               "vX.Y.Z-rc.N on staging for a release candidate (e.g. v3.0.3, v3.0.3-rc.2). " +
               "Nothing was published. Delete the tag with: git push --delete origin $tag")
    }
    $version = ConvertTo-SemVer $tag
    if ($version.Core -ne $declared.Text) {
        throw ("Tag '$tag' is for $($version.Core) but VERSION says $($declared.Text). Nothing was published. " +
               "Set VERSION on staging first, then tag; or delete the tag with: git push --delete origin $tag")
    }
    $kind           = 'release'
    $requiredBranch = if ($version.Prerelease) { 'staging' } else { 'main' }
}
else {
    $kind = if ($Ref -eq 'refs/heads/staging') { 'beta' } else { 'ci' }
    $version = ConvertTo-SemVer "$($declared.Text)-$kind.$RunNumber"
}

$prerelease = [bool]$version.Prerelease
$npmTag     = if ($prerelease) { 'next' } else { 'latest' }
$outputs = [ordered]@{
    version         = $version.Text
    npm_version     = Get-NpmVersion $version
    prerelease      = "$prerelease".ToLowerInvariant()
    npm_tag         = $npmTag
    kind            = $kind
    required_branch = $requiredBranch
}
$outputs.GetEnumerator() | ForEach-Object { "$($_.Key)=$($_.Value)" }
