# Releasing

How a version of legacycore (`libcci`/`libccic` native libraries, the
CoreConductor C# wrapper) gets built and shipped. For building and the CI
pipeline see [README.md](README.md#building-from-source); for vulnerabilities see
[SECURITY.md](SECURITY.md).

One tag releases everything. libcore and CoreConductor are published with the
same version at the same time:

| Where | What | When |
| --- | --- | --- |
| GitHub Releases | `build.zip` (the bare DLLs), `SHA256SUMS.txt` | every release tag |
| GitHub Packages (NuGet) | `ContactCI.Maestro.libcore`, `ContactCI.Maestro.CoreConductor` | every release tag |
| nuget.org | the same two packages | `RELEASE_NUGET` variable is `true` |
| npmjs | `@contactci/com.contactci.libcore`, `@contactci/com.contactci.coreconductor` | `RELEASE_NPM` variable is `true` |

nuget.org and npmjs are public. With those variables on, a pushed tag is a
public release.

## How the version is decided

The tag is the version. [`tools/resolve-version.ps1`](tools/resolve-version.ps1)
turns the ref into a version and the pipeline passes it to `build.ps1`, which
writes it into every package and into CoreConductor's assembly version.

There is nothing to bump. The versions in `.nuspec`, `CoreConductor/.nuspec`,
`package.json` and `CoreConductor/package.json` are placeholders and are
overwritten when packing.

Check what a tag would produce before pushing it:

```powershell
.\tools\resolve-version.ps1 -Ref refs/tags/2.3.13
```

### Tag format

Tags must be a semantic version: `X.Y.Z`, or `X.Y.Z-suffix` for a prerelease. A
leading `v` is accepted and stripped, but the convention in this repository is
the bare version (`2.3.12`, `3.0.2-alpha`). Anything else - `2.3`, `2.3.01`,
`2.3.1+build`, `release-2.3` - fails the pipeline before anything is built or
published.

A full fat release fit for normal consumption should be just the version:
`2.1.4`. If you're testing something, use `2.1.56-beta`, `2.1.58-dev`, `-rc` or
similar. **Do not** use feature names like `2.1.4-wav-stream`: the tag is visible
in the public package indexes, and it should neither confuse users about what
they're getting nor leak anything we don't have to.

A suffixed tag becomes a GitHub *prerelease*, and its npm packages go to the
`next` dist-tag rather than `latest`, so a plain `npm install` keeps resolving
the newest stable version.

### npm prerelease versions

For continuity with what GitLab published, a prerelease whose suffix doesn't end
in a number gets `.0` appended on npm: `2.1.15-beta` is published to npm as
`2.1.15-beta.0`. NuGet gets `2.1.15-beta` unchanged. `2.4.0-rc.2` already ends in
a number and is the same on both.

### CoreConductor's dependency on libcore

Written into the CoreConductor packages at pack time, from the version being
released:

| Release | NuGet dependency | npm dependency |
| --- | --- | --- |
| `2.3.13` | `[2.3.13, 2.4.0)` | `~2.3.13` |
| `2.4.0-beta` | `[2.4.0-beta]` | `2.4.0-beta.0` |

A stable release depends on the same minor version of libcore. A prerelease is
pinned exactly, because NuGet and npm never resolve a prerelease from a plain
range. Both packages are always released together, so the lower bound always
exists - the "you MUST make sure the lower end of your range exists" rule from
the GitLab days no longer needs a human.

The range in `CoreConductor.csproj` only affects IDE builds. Update it when the
minor version moves, so developers restore a libcore that matches the code.

## Procedure

### 1. Pick the version

```powershell
git fetch --tags
git tag --sort=-v:refname | Select-Object -First 5
```

Patch for fixes, minor for new API, major for a break. The C++ API and the C#
wrapper are published with the same version at the same time, so a new C++ API
ships with its C# wrapper in the same release - don't add one without the
other.

A protocol change - anything in `cci/packets` - usually means a matching
release of the [communication](https://github.com/Contact-Control-Interfaces/communication)
repository and of the Windows service. Land and bump the submodule first, and
know which service version the new libraries need.

### 2. Land everything that ships

Merge to `staging` through a pull request with a green pipeline. Each push to
`staging` publishes an `X.Y.Z-staging.N` NuGet prerelease to GitHub Packages
(see [Staging prereleases](#staging-prereleases)); that is the build to try in a
real client before tagging.

Check the `security` job even though it doesn't block. Nobody else will.

### 3. Verify the build from a clean tree

Tag what you have actually built:

```powershell
git checkout staging
git pull
git submodule update --init --recursive
powershell -ExecutionPolicy Bypass -File .\install.ps1
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Version 2.3.13 -AllTargets
```

There are no automated tests. The executables under `test\` are one-off tools
that need a running service, so a manual pass is part of the release. At
minimum, against a running service with a glove connected:
`build\release-mingw\test_cci.exe` and `test_ccic.exe` start a session and read
haptics, and a CoreConductor client (`HaptionReplay`, or your own) starts and
stops a session cleanly.

### 4. Tag

```bash
git tag 2.3.13
git push origin 2.3.13
```

An annotated tag works the same. Any commit can be tagged - there is no branch
gate, because the historical release tags sit on commits that no branch
contains.

### 5. Watch the pipeline

| Job | Does |
| --- | --- |
| `build` | builds and packages with the tag's version; a malformed tag fails here |
| `release` | creates the GitHub Release with `build.zip` and `SHA256SUMS.txt`, and generated notes |
| `nuget` | pushes both packages to GitHub Packages, then to nuget.org if `RELEASE_NUGET` is on |
| `npm` | publishes libcore, then CoreConductor, to npmjs if `RELEASE_NPM` is on |

`nuget` and `npm` start only after `release` succeeds, and they publish the
exact files `build` produced. A re-run skips versions that are already
published, so re-running a partly failed release finishes it rather than
failing on the half that worked.

### 6. Check what shipped

- The GitHub Release lists `build.zip` and `SHA256SUMS.txt`, and is marked as a
  prerelease if and only if the tag has a suffix.
- `build.zip` holds `build/release-mingw/libcci.dll` and `libccic.dll` - the
  layout GitLab releases had.
- On npmjs the packages show a provenance badge linking back to this run.
- A fresh project can install CoreConductor from each registry you published to,
  and its libcore dependency resolves.

Review the generated release notes and add what the commit log doesn't carry: a
required service version, a protocol change, or upgrade caveats.

### Signing

The DLLs and assemblies are **not Authenticode-signed**, and `build.ps1` has no
signing step - unlike the service and the Manager. `SHA256SUMS.txt` on the
release covers download integrity, and npm provenance covers the npm packages,
but nothing on a customer's machine can tell a genuine `libcci.dll` from a
substitute. See [SECURITY.md](SECURITY.md#supply-chain-and-build-integrity).

## Creating a tag without a release

Put `[skip ci]` or `[ci skip]` in an **annotated** tag's message:

```bash
git tag 3.0.0-rc -m "<optional message> [skip ci]"
```

The pipeline still builds the tag, so it is still checked, but the release and
publishing jobs are skipped. Use a suffix like `-rc` on such tags as well, so
it's obvious that the tag has no release. (On a lightweight tag there is no
message, so this does nothing.)

## Staging prereleases

Every push to `staging` publishes `X.Y.Z-staging.N` NuGet packages to GitHub
Packages. `N` is the workflow run number. `X.Y.Z` comes from the highest tag:
its next patch when that tag is stable (`2.3.12` -> `2.3.13-staging.N`), its own
version when it is a prerelease (`3.0.2-alpha` -> `3.0.2-staging.N`). Either way
it sorts above the last release and below the next one.

The newest 10 staging versions are kept and older ones deleted. `-alpha`,
`-beta` and every other version are never touched. Staging builds go nowhere
public.

## Hotfixes

Same procedure: branch from `staging` with a `bugfix/` prefix, merge, tag the
next patch version. To patch an older line while `staging` has moved on, branch
from that release's tag, fix, and tag the branch - there is no branch gate.

If the fix is for a security issue, read
[SECURITY.md](SECURITY.md#handling-a-confirmed-vulnerability) first: keep
exploit detail out of the public commit message and pull request until the
fixed version has shipped.

## Deleting a release

Delete the release and the tag on GitHub, and the package versions under the
organisation's Packages page. Versions on nuget.org can only be unlisted, never
deleted, and npm versions can only be unpublished within 72 hours - and a
version number, once used, can never be published again on either. Contact an
admin for guidance. Prefer shipping a fixed patch version over deleting one.

## If something goes wrong

| Symptom | What it means |
| --- | --- |
| `Tag '...' is not a release version` | The tag is not `X.Y.Z` / `X.Y.Z-suffix`. Nothing was published. `git push --delete origin <tag>` and push a correct one |
| `No .proto files in cci\packets\proto` | The communication submodule did not check out: the GitHub App (`SUBMODULE_APP_ID` / `SUBMODULE_APP_PRIVATE_KEY`) is missing or not installed on both repositories, and there is no `SUBMODULE_TOKEN` |
| `protobuf in ... does not match the current compiler` | MSYS2's gcc changed since protobuf was built. Run `install.ps1` |
| `libcci.dll imports MinGW runtime DLLs` | The `-static` link options were lost; the DLL would fail on any machine without MSYS2. Fix the CMake link options, do not ship |
| `ar.exe: ... No such file or directory` while building protobuf | Windows' 260-character path limit. Set `TMPDIR` to a short directory and re-run `install.ps1` |
| `RELEASE_NUGET is true but the NUGET_PUBLISH_KEY secret is not set.` | Add the secret, then re-run the `nuget` job |
| `403` pushing to GitHub Packages | The package exists but does not grant this repository write access: package settings -> Manage Actions access -> add `legacycore` with Write (Admin for staging pruning) |
| npm `E404` / `ENEEDAUTH` on publish | No trusted publisher configured for that package on npmjs, or its settings don't match `Contact-Control-Interfaces` / `legacycore` / `ci.yml` |
| npm `E403 ... cannot publish over` | That version is already on npmjs. The job checks for this; seeing it means the version string differs from what the check looked for |
