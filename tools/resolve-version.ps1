#Requires -Version 5.1
<#
    resolve-version.ps1
    ---------------------------------------------------------------
    Decides the package version for a CI run from the git ref, and prints
    it as key=value lines for $GITHUB_OUTPUT. Kept out of the workflow so
    the rules can be run and checked locally:

        .\tools\resolve-version.ps1 -Ref refs/tags/2.3.13
        .\tools\resolve-version.ps1 -Ref refs/heads/staging -RunNumber 42

    Rules
      refs/tags/<tag>     kind=release. The tag must be X.Y.Z or X.Y.Z-suffix,
                          optionally with a leading v; anything else throws,
                          so a mistyped tag fails before anything publishes.
                          An annotated tag whose message contains [skip ci] or
                          [ci skip] sets skip_release=true (a tag without a
                          release, as on GitLab).
      refs/heads/staging  kind=staging, version <base>-staging.<run number>
      anything else       kind=ci,      version <base>-ci.<run number>

    <base> comes from the highest semver tag in the repository: its next patch
    when that tag is stable (2.3.12 -> 2.3.13), its own X.Y.Z when it is a
    prerelease (3.0.2-alpha -> 3.0.2). Either way the result sorts above the
    last release and below the next one.

    Outputs: version, npm_version, prerelease, npm_tag, kind, skip_release
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

function ConvertTo-SemVer([string]$Text) {
    if ($Text -notmatch $SemVerPattern) { return $null }
    [pscustomobject]@{
        Text       = $Text -replace '^v', ''
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

$skipRelease = $false
if ($Ref -match '^refs/tags/(.+)$') {
    $tag = $Matches[1]
    $version = ConvertTo-SemVer $tag
    if (-not $version) {
        throw ("Tag '$tag' is not a release version. Release tags are X.Y.Z or X.Y.Z-suffix " +
               "(e.g. 2.3.13, 2.4.0-beta), optionally with a leading v. Nothing was published. " +
               "Delete the tag with: git push --delete origin $tag")
    }
    $kind = 'release'

    # for-each-ref rather than cat-file: it prints nothing for a missing ref
    # instead of writing to stderr, which Windows PowerShell 5.1 turns into a
    # terminating error. actions/checkout keeps annotated tags as tag objects
    # only with fetch-depth 0.
    $tagRef = "refs/tags/$tag"
    if ((& git -C $RepoDir for-each-ref --format='%(objecttype)' $tagRef) -eq 'tag') {
        $message = (& git -C $RepoDir for-each-ref --format='%(contents)' $tagRef) -join "`n"
        if ($message -match '\[(skip ci|ci skip)\]') { $skipRelease = $true }
    }
}
else {
    $kind = if ($Ref -eq 'refs/heads/staging') { 'staging' } else { 'ci' }
    $highest = Get-HighestTag
    $base = if (-not $highest)          { '0.0.1' }
            elseif ($highest.Prerelease) { "$($highest.Major).$($highest.Minor).$($highest.Patch)" }
            else                         { "$($highest.Major).$($highest.Minor).$($highest.Patch + 1)" }
    $version = ConvertTo-SemVer "$base-$kind.$RunNumber"
}

$prerelease = [bool]$version.Prerelease
$npmTag     = if ($prerelease) { 'next' } else { 'latest' }
$outputs = [ordered]@{
    version      = $version.Text
    npm_version  = Get-NpmVersion $version
    prerelease   = "$prerelease".ToLowerInvariant()
    npm_tag      = $npmTag
    kind         = $kind
    skip_release = "$skipRelease".ToLowerInvariant()
}
$outputs.GetEnumerator() | ForEach-Object { "$($_.Key)=$($_.Value)" }
