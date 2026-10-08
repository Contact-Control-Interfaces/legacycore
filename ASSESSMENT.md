# Manual build - inventory assessment

What used to be a manual prerequisite for building, packaging or shipping
legacycore (`libcci`, `libccic` and the CoreConductor C# wrapper), and what now
does it instead.

"Was" comes from the GitLab-era `BUILDING.md` and `.gitlab-ci.yml` (both in git
history). "Now" was verified by running it, unless the row says otherwise.

Entry points:

| Command | Does |
| --- | --- |
| `.\install.ps1` | Provisions the toolchain on a clean Windows machine; no elevation needed |
| `.\build.ps1` | Builds the native DLLs and CoreConductor, checks the DLLs, packages everything a release publishes |
| `.\tools\resolve-version.ps1 -Ref <ref>` | Shows the version CI would build for a tag or branch |
| `git push origin X.Y.Z` | Builds, releases and publishes through `.github/workflows/ci.yml` |

There is no `Dockerfile` here, unlike windows-service and the Manager. The
toolchain is MSYS2, which CI provisions with `msys2/setup-msys2`.

---

## 1. Toolchain installation - `install.ps1`

| Prerequisite | Was | Now |
| --- | --- | --- |
| MSYS2 | The GitLab job downloaded the *nightly* installer into the project directory and cached it per branch. Developers: install it somehow, unstated | Downloaded if absent. CI uses `msys2/setup-msys2`, and `install.ps1` then only verifies |
| MINGW64 gcc, cmake, ninja | `pacman -Syuu` plus a package list, on every pipeline. Developers: unstated | Installed only when one is missing, as `pacman -Syu --needed` - never a partial upgrade |
| protobuf | Unzip `protobuf.zip` (prebuilt, GCC 13) and point CLion's `CMAKE_PREFIX_PATH` at it by hand. `BUILDING.md` said 3.22; the zip is 3.21.12 | `tools/build-protobuf.sh` builds 3.21.12 from source, SHA-256-verified, and stamps the gcc it was built with. It's rebuilt when gcc changes; CI caches it per gcc version |
| .NET 8 SDK | `CoreConductor/global.json` pins 8.0.x - assumed present, discovered missing when `dotnet build` failed | Installed per user if no 8.x SDK is found |
| nuget.exe | Downloaded into a `cache\` folder by the GitLab install job | Used from PATH, else downloaded to `C:\Tools` |
| Node / npm | MSYS2's `nodejs` installed on every pipeline | Used from PATH; MSYS2's `nodejs` only when there is none |
| "Am I elevated?" | n/a | Nothing needs elevation |
| "Did it work?" | Read the scrollback | Exit code `0` ready, `1` failed with the failing step named |

**Evidence:** a cold run into an empty directory downloaded MSYS2, installed the
packages, built protobuf against MSYS2's GCC 16.2.0 (`libprotoc 3.21.12`), and
installed .NET SDK 8.0.425. A second run reported every component present and
exited 0 in 3 seconds without downloading anything.

```powershell
powershell -ExecutionPolicy Bypass -File .\install.ps1
```

---

## 2. Build and package - `build.ps1`

| Step | Was | Now |
| --- | --- | --- |
| Submodule | `git submodule update` in each job's `before_script`, over SSH with a runner-held private key | Checkout with a GitHub App token scoped to this repo and `communication`. `build.ps1` asserts the `.proto` files are there and says how to fix it |
| Configure | A CLion CMake profile, or an ad hoc `cmake` line | `cmake --fresh -G Ninja` into `build\release-mingw`, so a stale CLion cache cannot leak in |
| Targets | `ccic` only; the `test\` executables were never compiled by CI | `ccic` by default; `-AllTargets` also compiles `test\`, and CI uses it |
| Static linking | Assumed | `objdump` asserts neither DLL imports `libstdc++`, `libgcc`, `libwinpthread` or `libprotobuf` - result: `KERNEL32.dll`, `msvcrt.dll` only |
| CoreConductor | `dotnet build` in its own job, restoring the libcore `PackageReference` from the GitLab feed | Built with `-p:SkipLibcorePackage=true`; no feed or credentials needed |
| Version | `nuget pack -version $TAG` / `npm version` repeated in each of eight release jobs; CoreConductor.dll always reported 1.0.0 | One `-Version`, written into every package and the CoreConductor assembly |
| CoreConductor -> libcore range | Hand-edit three strings (csproj, nuspec, package.json) on every minor bump, and remember that the lower bound must exist | Written at pack time: `[X.Y.Z, X.(Y+1).0)` / `~X.Y.Z`, or an exact pin for a prerelease |
| Packaging | Each release job packed its own package; `build.zip` was zipped in a separate Alpine job | One step: `build.zip`, two nupkgs, two npm tarballs, `SHA256SUMS.txt` |

**Evidence:** `build.ps1 -Version 2.3.13 -AllTargets` and
`build.ps1 -Version 2.4.0-beta -NpmVersion 2.4.0-beta.0` both pass in about 30
seconds on a provisioned machine.

### Compared with what GitLab published

`build.ps1 -Version 2.3.12` against the 2.3.12 packages on nuget.org and npmjs:

| | Result |
| --- | --- |
| NuGet file lists | Identical, apart from `.signature.p7s`, which nuget.org adds on ingest |
| `libcci.dll` / `libccic.dll` exports | Same names (63 / 22); `contactci.h` byte-identical |
| Dependency ranges | `[2.2.30, 2.4.0)` / `~2.3.1` become `[2.3.12, 2.4.0)` / `~2.3.12` - intended |
| `repository` metadata | Added to both nuspecs and both package.json files - intended |
| npm tarballs | **`README.md` missing** - open, see below |
| libcore nuspec `<copyright>` | **`Â©` instead of `©`** - open, see below |

### Defects this uncovered

Found by running the build, not by reading it:

- **`protobuf.zip` no longer works.** Its `protoc.exe` does not start against
  current MSYS2 (exit `0xC0000139`, a libstdc++ entry point that no longer
  exists), so every build with an up-to-date MSYS2 failed at the first `.proto`.
  It has been removed from the repository.
- **The GitLab MSVC job could not have built.** It ran `cmake --build .` from
  the repository root, not from `build/release-msvc`. MSVC builds are not
  used, so the GitHub pipeline has no MSVC job; only MinGW builds ship.
- **The public publishing jobs were gated on a variable tag pipelines don't
  set.** The nuget.org and npmjs jobs had `if: $CI_COMMIT_BRANCH != "master"` ->
  `never`, and GitLab leaves `CI_COMMIT_BRANCH` unset in tag pipelines. Versions
  up to 2.3.12 are on both registries, so they reached them some other way - the
  record of how is not in this repository.
- **Tags were never validated.** `BUILDING.md` said CI would reject a
  malformed tag. No job checked; the only regex picked prereleases for npm's
  `.0` suffix.
- **A fresh MSYS2 can ship an empty MINGW64 CA bundle**, so MINGW64's `curl`
  fails every HTTPS download (error 77). `tools/build-protobuf.sh` uses MSYS's
  own curl.

### One thing worth recording

protobuf's build tree is about 120 characters deep. Under an MSYS2 installed at
a long path, the object paths pass Windows' 260-character limit and `ar.exe`
reports `No such file or directory` for a file that exists. `C:\msys64` and CI's
`D:\a\_temp\msys64` are short enough; otherwise point `TMPDIR` at a short
directory.

---

## 3. Testing

| | Was | Now |
| --- | --- | --- |
| Unit tests | None | None |
| `test\` executables | Built by hand, if at all | Compiled on every pipeline (`-AllTargets`), so they can't rot. Running them needs a live service and a glove |

Nothing here is covered by automated tests. The libraries talk to the service
over a named pipe and shared memory, so a meaningful test needs either the
service or a fake of it - neither exists yet.

---

## 4. Security and supply chain - the `security` job, the `sbom` job

| Check | Was | Now |
| --- | --- | --- |
| Secret scanning | None | gitleaks 8.28.0 over full history on every pipeline; redacted report kept 30 days |
| protobuf provenance | Prebuilt binaries of unrecorded origin, in `protobuf.zip` | Pinned source release, SHA-256-verified, built in the pipeline |
| CI credentials | A runner-held SSH private key for submodules; job tokens; `CI_NPM_PUBLISH_KEY` | GitHub App token scoped to two repositories; npm trusted publishing (OIDC, no token, with provenance); least-privilege `permissions:` per job; no persisted checkout credentials |
| Trust boundary | Undocumented | [SECURITY.md](SECURITY.md) records what the libraries trust from the pipe and shared memory |
| Third-party notices | Not shipped, not recorded | Recorded in [LICENSE.md](LICENSE.md) as still missing from the packages |
| SBOM | None | CycloneDX 1.5 `sbom/bom.json` from `tools/write-sbom.ps1`, written by every build. It covers the five artifacts and both DLLs, plus protobuf (pinned version and source hash), the communication commit, and the MSYS2 packages that own the static archives gcc links (gcc, winpthreads, crt), with versions and SPDX licences from pacman. The `sbom` job validates it with cyclonedx-cli and checks every hash against `artifacts/`; kept 90 days. syft is not used: the statically linked components leave no metadata for it to find |
| Dependency audit | None | Still none. CoreConductor references no packages and the native side has no manifest, so there is nothing for a NuGet audit to read. Recorded as a known absence |

The `security` and `sbom` jobs are `continue-on-error: true`: they report, they do not block.

---

## 5. Releasing - the `release`, `nuget`, `npm` and `staging` jobs

| Step | Was | Now |
| --- | --- | --- |
| Tag check | None (see above) | `resolve-version.ps1` rejects anything but `X.Y.Z` / `X.Y.Z-suffix` before anything is built |
| Release page | GitLab release linking a generic-package upload | GitHub Release with `build.zip`, `SHA256SUMS.txt` and generated notes; marked prerelease for suffixed tags |
| Internal feed | GitLab NuGet and npm registries | GitHub Packages (NuGet). Internal npm is dropped: GitHub's npm registry only hosts a scope matching the owner |
| Public registries | Tokens; gated as described above | nuget.org with `NUGET_PUBLISH_KEY`, npmjs with OIDC; switched on by `RELEASE_NUGET` / `RELEASE_NPM` |
| npm dist-tag | Everything went to `latest` | Prereleases go to `next` |
| Staging builds | None | Every push to `staging` publishes `X.Y.Z-staging.N` to GitHub Packages; the newest 10 are kept |
| Re-running a failed release | Failed on whatever had already been published | Already-published versions are skipped |
| Rebuilding between test and publish | Each release job used the build stage's artifacts, but packed them itself | Publishing jobs push the exact files the `build` job produced |

[RELEASE.md](RELEASE.md) has the procedure.

---

## What is still manual

On purpose:

- **Trying a build against hardware before tagging.** CI runners have no service
  and no gloves. [RELEASE.md](RELEASE.md) step 3 says what to run.
- **Reading the `security` job.** It doesn't fail the branch; someone has to
  look.
- **Adding context to release notes** that the commit log doesn't carry.

Not on purpose - open items:

- **None of the GitHub jobs has run yet.** `ci.yml` passes actionlint, and
  everything it calls was run locally, but the workflow itself, the GitHub App
  token, the protobuf cache, publishing and staging pruning are unverified until
  the first push.
- **One-time setup:**
  - an npmjs trusted publisher on each `@contactci` package
  - `NUGET_PUBLISH_KEY`, then `RELEASE_NUGET` / `RELEASE_NPM`
  - the GitHub App installed on this repository as well as `communication`
  - write (Admin for pruning) access for this repository on the existing GitHub Packages
  - private vulnerability reporting switched on
- **Two packaging regressions against 2.3.12**: the npm tarballs omit
  `README.md`, and the libcore nuspec's `©` is mis-encoded by Windows PowerShell
  5.1. Both are in `build.ps1` and must be fixed before the first public
  release.
- **Third-party notices** for protobuf and winpthreads are not shipped in the
  packages ([LICENSE.md](LICENSE.md)).
- **The pipe and shared-memory trust issues** in [SECURITY.md](SECURITY.md) are
  documented, not fixed.
- **No code signing.** The DLLs and CoreConductor are unsigned, and there is no
  signing mechanism here yet, unlike the service and the Manager.
- **The toolchain is not pinned.** MSYS2 packages are whatever is current when
  the pipeline runs, so two builds of one commit can differ.
- **protobuf 3.21.x is out of upstream support.** Moving past it means taking on
  Abseil.
