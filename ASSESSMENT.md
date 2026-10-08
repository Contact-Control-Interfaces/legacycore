# Manual build - inventory assessment

What used to be a manual prerequisite for building, packaging or shipping
legacycore (`libcci`, `libccic` and the CoreConductor C# wrapper), and what now
does it instead.


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
| MSYS2 | The GitLab job downloaded the nightly installer into the project directory and cached it per branch | Downloaded if absent |
| MINGW64 gcc, cmake, ninja | `pacman -Syuu` on every pipeline. | Installed only when one is missing with `pacman -Syu --needed` |
| protobuf | Manually unzip `protobuf.zip` (prebuilt, GCC 13) and point CLion's `CMAKE_PREFIX_PATH` | avoid update break with `tools/build-protobuf.sh` |
| .NET 8 SDK | `CoreConductor/global.json` pins 8.0.x | Installed if no 8.x SDK is found |
| nuget.exe | Downloaded into a `cache\` by the GitLab job | Used from PATH |
| Node / npm | MSYS2's `nodejs` installed on every pipeline | Used from PATH or install if missing |


```powershell
powershell -ExecutionPolicy Bypass -File .\install.ps1
```

---

## 2. Build and package - `build.ps1`

| Step | Was | Now |
| --- | --- | --- |

| Targets | `ccic`; the `test\` executables were never compiled by CI | automatic tests |
| Version | `nuget pack -version $TAG` repeated in each release | automatic |
| CoreConductor, libcore | Hand-edit three strings (csproj, nuspec, package.json) on every bump | Written at pack time |
| Packaging | Each release job packed its own package and alpine job for build.zip | on release job for all |


```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Version 2.3.13 -IncludeTests
```

---

## 3. Testing

| | Was | Now |
| --- | --- | --- |
| Unit tests | None | 66 GoogleTest cases in `test\unit`, JUnit XML per run; the CI `test` stage blocks staging and releases |
| Hardware tests | Run the `test\` tools by hand, watch the glove | 4 device tests, reported **skipped** without hardware; `CCI_DEVICE_TESTS=1` runs them on a lab machine |
| `test\manual` tools (were `test\`) | built by hand | compiled on CI; still for hands-on checks |

### Defects the tests found

| Where | Defect | Status |
| --- | --- | --- |
| `value_monitor.h` | Each session's device and client monitor threads were started in the base-class constructor and joined in the base-class destructor, so they could run derived-class code before the object was built or after it was destroyed. This made a session crash (segfault) on open or close about once per 1,000 sessions, and it is in released 2.3.12 and 3.0.2-alpha | **Fixed**: threads start last in the derived constructors and stop first in the derived destructors. `SessionTest.SurvivesRepeatedOpenAndClose` is the regression test |
| `value_monitor.cpp` | The left/right glove was kept in members the monitor thread wrote without a lock while callers read them, and could read "no device" straight after a session opened | **Fixed**: derived from the locked device list, waiting for the first fetch. `SessionTest.LeftAndRightAreKnownAsSoonAsTheSessionOpens` |
| `value_monitor.h` | If the service disconnected during a session's first device fetch, the monitor thread recorded the error but never released callers waiting for the first value: `get_device_list()` / `get_left_device()` hung forever. Its mutex also stayed locked if a fetch threw | **Fixed**: a failed first fetch releases waiters, who get the error; fetches happen outside the lock. `SessionTest.ServiceDroppingDuringTheFirstFetchRaisesInsteadOfHanging` (hangs without the fix) |
| `channel.cpp` | Requests took the channel's spinlock with `lock()` / `unlock()`, so a failed pipe read left it held and every later request on the session spun forever | **Fixed**: `std::lock_guard`. `Channel.FailedRequestReleasesTheLock` |
| `error.h` | `class Exception : std::runtime_error` inherits privately, so `catch (const std::exception &)` never catches libcci errors | Open, pinned by `Exception.KnownDefect_NotCatchableAsStdException` |
| `channel.cpp` | `ParseFromString` results are ignored: a truncated response yields a phantom, empty device instead of an error | Open, pinned by `Channel.KnownDefect_MalformedResponseIsSilentlyAccepted` |

---

## 4. Security and supply chain

| Check | Was | Now |
| --- | --- | --- |
| Secret scanning | None | gitleaks 8.28.0  |
| CI credentials | A runner-held SSH private key for submodules and job tokens | GitHub App token scoped, npm trusted publishing, least-privilege, no persisted checkout credentials |
| Trust boundary | Undocumented | [SECURITY.md](SECURITY.md)  |
| Third-party notices | Not shipped | Recorded in [LICENSE.md](LICENSE.md) |
| SBOM | None | automatic |


---

## 5. Releasing - the `release` job

The pipeline is one straight line, `build` → `test` → `security` → `sbom` → `release`, so nothing is published unless every gate passed.

| Step | Was | Now |
| --- | --- | --- |
| Pipeline shape | Release, NuGet and npm jobs fanned out per registry, internal and public | One `release` job publishes everything, after the gates |
| Tag check | None | `resolve-version.ps1` rejects anything but `X.Y.Z` / `X.Y.Z-suffix` before anything is built |
| Release page | GitLab release linking a generic-package upload | GitHub Release with `build.zip`, `SHA256SUMS.txt` and generated notes |
| Public registries | Tokens | nuget.org with `NUGET_PUBLISH_KEY`, npmjs with OIDC; switched on by `RELEASE_NUGET`or `RELEASE_NPM` |
| Staging builds | None | Every push to `staging` publishes `X.Y.Z-staging.N` to GitHub Packages |

[RELEASE.md](RELEASE.md) has the procedure.

---

## What is still manual

- **One-time setup:**
  - an npmjs trusted publisher on each `@contactci` package
  - `NUGET_PUBLISH_KEY`, then `RELEASE_NUGET` or  `RELEASE_NPM`
  - the GitHub App installed on this repository as well as `communication`
  - write (Admin for pruning) access for this repository on the existing GitHub Packages
  - private vulnerability reporting switched on
- **The pipe and shared-memory trust issues** in [SECURITY.md](SECURITY.md) are
  documented.
