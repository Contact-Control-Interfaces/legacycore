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

| Targets | `ccic`; the `test\` executables were never compiled by CI | `ccic` and also run tests |
| Version | `nuget pack -version $TAG` repeated in each release | automatic |
| CoreConductor, libcore | Hand-edit three strings (csproj, nuspec, package.json) on every bump | Written at pack time |
| Packaging | Each release job packed its own package and alpine job for build.zip | on release job for all |


```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Version 2.3.13 -AllTargets
```

---

## 3. Testing

| | Was | Now |
| --- | --- | --- |
| Unit tests | None | None |
| `test\` executables | built by hand | automatic on CI |

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

## 5. Releasing - the `release`, `nuget`, `npm` and `staging` jobs

| Step | Was | Now |
| --- | --- | --- |
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
