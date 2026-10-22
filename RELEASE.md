# Releasing

How a version of legacycore (`libcci`/`libccic` native libraries for Windows
and Linux, the CoreConductor C# wrapper) gets built and shipped. For building and the CI
pipeline see [README.md](README.md#building-from-source); for vulnerabilities see
[SECURITY.md](SECURITY.md).

One tag releases everything. libcore and CoreConductor are published with the
same version at the same time:

| Where | What | When |
| --- | --- | --- |
| GitHub Releases | `build.zip` (the bare DLLs), `SHA256SUMS.txt` | every release tag |
| GitHub Releases | Linux, per Ubuntu release (22.04, 24.04) and architecture (amd64, arm64): `libcontactci-core3_*.deb`, `libcontactci-core-dev_*.deb`, `contactci-core_*.tar.gz` | every release tag |
| GitHub Packages (NuGet) | `ContactCI.Maestro.libcore`, `ContactCI.Maestro.CoreConductor` | every release tag |
| nuget.org | the same two packages | `RELEASE_NUGET` variable is `true` |
| npmjs | `@contactci/com.contactci.libcore`, `@contactci/com.contactci.coreconductor` | `RELEASE_NPM` variable is `true` |

nuget.org and npmjs are public. With those variables on, a pushed tag is a
public release.

## How the version is decided

The rules are the [dev-handbook's](https://github.com/Contact-Control-Interfaces/dev-handbook/blob/main/releases/versioning.md);
[`tools/resolve-version.ps1`](tools/resolve-version.ps1) applies them and the
pipeline passes the result to `build.ps1`, which writes it into every package and
into CoreConductor's assembly version:

| Where the build runs | Version | Tag | Published to |
| --- | --- | --- | --- |
| pull request, `main`, other branch | `X.Y.Z-ci.N` | none | workflow artifacts |
| push to `staging` | `X.Y.Z-beta.N` | none | NuGet on GitHub Packages, newest 10 kept; Linux packages as workflow artifacts |
| `vX.Y.Z-rc.N` tag on a `staging` commit | `X.Y.Z-rc.N` | yours | draft GitHub prerelease, NuGet, npm dist-tag `next` |
| `vX.Y.Z` tag on a `main` commit | `X.Y.Z` | yours | draft GitHub release, NuGet, npm `latest`, public registries if opted in |

`X.Y.Z` is the `VERSION` file at the repo root: the version `staging` is working
towards. The versions in `.nuspec`, `CoreConductor/.nuspec`, `package.json` and
`CoreConductor/package.json` are placeholders that `build.ps1` overwrites at pack
time. Bump `VERSION` on `staging` right after a release.

The Linux packages carry the same version. Their file names use it as is
(`libcontactci-core3_3.0.3-rc.1_ubuntu22.04_amd64.deb`); inside the `.deb` a
prerelease is `3.0.3~rc.1`, because dpkg sorts `~` below the release, as SemVer
sorts a prerelease. The runtime package is named after the SONAME major version
(`libcontactci-core3` ships `libccic.so.3`), so a major version bump renames it
in `debian/control`; `build.sh --package` fails until the two agree.

Check what a ref would produce before tagging:

```powershell
.\tools\resolve-version.ps1 -Ref refs/tags/v3.0.3-rc.2
.\tools\resolve-version.ps1 -Ref refs/heads/staging -RunNumber 42
```

### Tag format

Only release candidates and releases are tagged, by a person: `vX.Y.Z-rc.N` on a
`staging` commit, `vX.Y.Z` on a `main` commit. The `v` prefix is required.
Anything else - `3.0.3`, `v3.0`, `v3.0.3-beta`, `v3.0.3-wav-stream` - fails the
pipeline before anything is built or published, as does a tag whose `X.Y.Z`
differs from `VERSION` or that isn't on its branch. Tags are permanent: a broken
rc is fixed by `rc.N+1`, a broken release by the next patch version.

An rc becomes a GitHub *prerelease*, and its npm packages go to the `next`
dist-tag rather than `latest`, so a plain `npm install` keeps resolving the
newest stable version.

The historical tags (`2.3.12`, `3.0.1-alpha`, `v1.0.0`) predate this standard and
are left alone; the version script still counts the unprefixed ones when it
checks that `VERSION` is above the last release.

### npm prerelease versions

For continuity with what GitLab published, a prerelease whose suffix doesn't end
in a number gets `.0` appended on npm. `rc.N` and `beta.N` already end in a
number and are the same on both registries.

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

Set `VERSION` on `staging` to the version you are releasing, and confirm
`cci/packets` pins the communication release tag you intend to ship (its
`main`; release communication first if the change is only on its `staging`).
Merge to `staging` through a pull request with a green pipeline. Each push to
`staging` publishes an `X.Y.Z-beta.N` NuGet prerelease to GitHub Packages (see
[Beta builds](#beta-builds)); that is the build to try in a real client before
tagging.

`security` and `sbom` are part of the gate: a gitleaks finding or an SBOM that
doesn't match the packages stops the pipeline before anything is published.

### 3. Verify the build from a clean tree

Tag what you have actually built:

```powershell
git checkout staging
git pull
git submodule update --init --recursive
powershell -ExecutionPolicy Bypass -File .\install.ps1
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Version 2.3.13 -IncludeTests
```

The automated tests (`test\unit`) must pass; `-IncludeTests` stops the build if
any fails, and CI's `test` job blocks the release the same way.

They run against a fake service, so a pass against real hardware is still part
of the release. On a machine with the service running and a glove connected:

```powershell
$env:CCI_DEVICE_TESTS = '1'
.\tools\run-tests.ps1
```

This runs the four device tests that CI reports as skipped. Also check that a
CoreConductor client (`HaptionReplay`, or your own) starts and stops a session
cleanly.

On Linux, build and package the same commit, and check the packages install
and link on a clean Ubuntu (needs docker):

```bash
./build.sh --all-targets --package
tools/check-linux-packages.sh
``` The `test\manual` tools (`build\release-mingw\test_cci.exe` and the
rest) are still there for anything that needs watching by hand.

### 4. Tag the release candidate, then promote it

```bash
git fetch origin
git tag -a v3.0.3-rc.2 -m "3.0.3-rc.2" origin/staging
git push origin v3.0.3-rc.2
```

The pipeline refuses an rc that isn't on `staging`. When the rc has been tried,
fast-forward `main` to the rc commit and tag the same commit, so the release is
the very build that was tested:

```bash
git checkout main
git merge --ff-only v3.0.3-rc.2
git push origin main
git tag -a v3.0.3 -m "3.0.3" v3.0.3-rc.2
git push origin v3.0.3
```

If `main` has commits `staging` lacks, bring them into `staging` first and cut a
new rc. Afterwards, bump `VERSION` on `staging` to the next version.

### 5. Watch the pipeline

`build` → `test` → `security` → `sbom` → `release`, each stage starting only if
the previous one passed, with `build-linux` alongside; `release` also waits for
every `build-linux` leg:

| Job | Does |
| --- | --- |
| `build` | builds and packages with the tag's version; a malformed tag, a tag that doesn't match `VERSION`, or a tag on the wrong branch fails here |
| `build-linux` | on each Ubuntu release and architecture: builds, packages with `build.sh --package`, and installs and uses the `.deb`s on a clean container (`tools/check-linux-packages.sh`) |
| `test` | runs `test\unit` against the DLLs `build` packaged |
| `security` | gitleaks over the full history |
| `sbom` | validates the SBOM and checks it matches the packages |
| `release` | in order: the GitHub Release with `build.zip`, the Linux `.deb`s and tarballs, `SHA256SUMS.txt` (the Windows files and the Linux ones) and generated notes; both NuGet packages to GitHub Packages, then to nuget.org if `RELEASE_NUGET` is on; libcore then CoreConductor to npmjs if `RELEASE_NPM` is on |

`release` publishes the exact files `build` and `build-linux` produced. A re-run skips versions
that are already published, so re-running a partly failed release finishes it
rather than failing on the half that worked.

### 6. Check what shipped

- The GitHub Release lists `build.zip` and `SHA256SUMS.txt`, and is marked as a
  prerelease if and only if the tag has a suffix.
- `build.zip` holds `build/release-mingw/libcci.dll` and `libccic.dll` - the
  layout GitLab releases had.
- There are eight `.deb`s and four tarballs, one runtime and one dev package
  and one tarball for each of `ubuntu22.04` and `ubuntu24.04` on `amd64` and
  `arm64`, and `SHA256SUMS.txt` lists every one of them.
- On a clean Ubuntu, `sudo apt install ./libcontactci-core3_<version>_ubuntu22.04_amd64.deb`
  installs, and with the dev package a program that includes `<contactci.h>`
  builds with `gcc app.c -lccic` (CI checks both on every leg).
- On npmjs the packages show a provenance badge linking back to this run.
- A fresh project can install CoreConductor from each registry you published to,
  and its libcore dependency resolves.

Review the generated release notes and add what the commit log doesn't carry: a
required service version, a protocol change, or upgrade caveats.

### Signing

The DLLs and assemblies are **not Authenticode-signed**, and `build.ps1` has no
signing step - unlike the service and the Manager. The Linux `.deb`s are not
signed either, and there is no apt repository to carry a signature. `SHA256SUMS.txt` on the
release covers download integrity, and npm provenance covers the npm packages,
but nothing on a customer's machine can tell a genuine `libcci.dll` from a
substitute. See [SECURITY.md](SECURITY.md#supply-chain-and-build-integrity).


## Beta builds

Every push to `staging` publishes `X.Y.Z-beta.N` NuGet packages to GitHub
Packages. `N` is the workflow run number and `X.Y.Z` is `VERSION`. SemVer sorts
`beta` below `rc`, so a client that accepts prereleases never picks a staging
build over a release candidate.

The Linux packages of a staging build (`X.Y.Z~beta.N` inside the `.deb`) are
workflow artifacts only, `linux-packages-<os>` on the run, kept 30 days. There
is no apt feed.

The newest 10 beta versions are kept and older ones deleted. Everything else is
never touched. Beta builds go nowhere public.

## Hotfixes

Same procedure: branch from `staging` with a `bugfix/` prefix, merge, set
`VERSION` to the next patch version, cut an rc, promote. There are no release
branches; `staging` and `main` are the only lines, and a tag must be on one of
them.

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
| A push or a tag starts no workflow run at all | The commit message carries a skip-CI marker (`[skip ci]`, `[ci skip]`, `[no ci]`, `[skip actions]`, `[actions skip]`). GitHub then creates no run for that push, and none for a tag on that commit either. Keep those strings out of commit messages and pull request bodies (a squash merge copies the body into the commit). Land another commit and tag that |
| `Tag '...' is not a release tag` | Release tags are `vX.Y.Z` and `vX.Y.Z-rc.N`. Nothing was published. `git push --delete origin <tag>` and push a correct one |
| `Tag '...' is for 3.0.4 but VERSION says 3.0.3` | Set `VERSION` on `staging` to the version you are releasing, then tag |
| `VERSION says 3.0.3 but 3.0.3 is already released` | Bump `VERSION` on `staging` to the next version |
| `v3.0.3-rc.2 is not on staging` | The tag is on a feature or pull request commit. Tag a commit on `staging`; a release tag goes on `main` after the fast-forward |
| `No .proto files in cci\packets\proto` | The communication submodule did not check out: the GitHub App (`SUBMODULE_APP_ID` / `SUBMODULE_APP_PRIVATE_KEY`) is missing or not installed on both repositories, and there is no `SUBMODULE_TOKEN` |
| `protobuf in ... does not match the current compiler` | MSYS2's gcc changed since protobuf was built. Run `install.ps1` |
| `debian/control has no libcontactci-coreN package` | `VERSION`'s major version changed, which changes the SONAME. Rename the runtime package in `debian/control` (and its `.install` file, and the dev package's dependency) to match |
| `Install and use the packages on a clean Ubuntu` fails in `build-linux` | The `.deb`s do not install, or `libccic.so.N` does not load, or `<contactci.h>` does not compile against the dev package. Run `./build.sh --package` and `tools/check-linux-packages.sh` on that Ubuntu release to reproduce |
| `libcci.dll imports MinGW runtime DLLs` | The `-static` link options were lost; the DLL would fail on any machine without MSYS2. Fix the CMake link options, do not ship |
| `ar.exe: ... No such file or directory` while building protobuf | Windows' 260-character path limit. Set `TMPDIR` to a short directory and re-run `install.ps1` |
| `RELEASE_NUGET is true but the NUGET_USER variable is not set.` | Set the `NUGET_USER` variable to the nuget.org profile name that owns the trusted publishing policy, then re-run the `release` job |
| `NuGet/login` fails to get an API key | The nuget.org trusted publishing policy is missing, inactive, or doesn't match `Contact-Control-Interfaces` / `legacycore` / `ci.yml` / no environment, or `NUGET_USER` isn't its owner. A new policy can start out temporarily active for 7 days and lapse if nothing is published in that time; restart its window on nuget.org |
| `403` pushing to GitHub Packages | The package exists but does not grant this repository write access: package settings -> Manage Actions access -> add `legacycore` with Write (Admin for staging pruning) |
| npm `E404` / `ENEEDAUTH` on publish | No trusted publisher configured for that package on npmjs, or its settings don't match `Contact-Control-Interfaces` / `legacycore` / `ci.yml` |
| npm `E403 ... cannot publish over` | That version is already on npmjs. The job checks for this; seeing it means the version string differs from what the check looked for |
