# Contact CI Library

This project houses the native C and C++ libraries (`libcci`, `libccic`) and CoreConductor, their C# wrapper. These libraries are meant to be consumed via a NuGet or npm package, so as to better maintain version compatibility..

Releasing is covered in [RELEASE.md](RELEASE.md), reporting a vulnerability in [SECURITY.md](SECURITY.md), and licensing in [LICENSE.md](LICENSE.md). [ASSESSMENT.md](ASSESSMENT.md) lists the manual build steps this tooling replaced, and what is still manual.

## Building from source

Builds are handled by GitHub Actions ([`.github/workflows/ci.yml`](.github/workflows/ci.yml)) and are available as NuGet and npm packages, as well as the bare DLLs. See the [Releases page](https://github.com/Contact-Control-Interfaces/legacycore/releases) if you need the bare DLLs.

For minor changes you don't actually need to be able to build. You can open a pull request and let CI build it. Creating a release is covered in [RELEASE.md](RELEASE.md).

Since the build environment is sensitive, `install.ps1` and `build.ps1` helps to assemble it automatically on a clean machine, and CI runs the same scripts for reproducibility.

### Quick start

```powershell
git clone --recurse-submodules https://github.com/Contact-Control-Interfaces/legacycore.git
cd legacycore
powershell -ExecutionPolicy Bypass -File .\install.ps1   # once; no elevation needed
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

`install.ps1` provisions or verifies if it's already there:

| What | Where | Notes |
| --- | --- | --- |
| MSYS2 | `C:\msys64` (`-MsysRoot` / `MSYS2_ROOT`) | downloaded if absent |
| MINGW64 gcc, cmake, ninja | inside MSYS2 | `pacman -Syu --needed`, only when it is missing |
| protobuf 3.21.12 | `.\protobuf` | built automatically |
| .NET 8 SDK | `%LOCALAPPDATA%\Microsoft\dotnet` | only if no 8.x SDK found |
| nuget.exe | `C:\Tools` | only if not on PATH |

`build.ps1` builds libcci/libccic and CoreConductor, then packages everything a release publishes into `.\artifacts`:

| File | Contents |
| --- | --- |
| `build.zip` | `build/release-mingw/libcci.dll`, `libccic.dll` |
| `ContactCI.Maestro.libcore.<ver>.nupkg` | the DLLs and `contactci.h` |
| `ContactCI.Maestro.CoreConductor.<ver>.nupkg` | the C# wrapper |
| `contactci-com.contactci.libcore-<ver>.tgz` | npm equivalent of the libcore package |
| `contactci-com.contactci.coreconductor-<ver>.tgz` | npm equivalent of the CoreConductor package |
| `SHA256SUMS.txt` | checksums of the above |

It also writes `.\sbom\bom.json`, a CycloneDX 1.5 SBOM of those files (`tools/write-sbom.ps1`). The SBOM covers what is linked into the DLLs (protobuf, the GCC runtime, winpthreads, the mingw-w64 CRT), the communication commit, and the hash of every artifact.

`build.ps1` checks that both DLLs import no MinGW runtime DLLs (`libstdc++`, `libgcc`, `libwinpthread`). They are linked `-static` so that customers need nothing but the DLLs, and a missing flag would otherwise only show up on a machine without MSYS2.

### Prerequisites
This repo uses the [communication repo](https://github.com/Contact-Control-Interfaces/communication) as a submodule, so always do `git submodule update --init --recursive` after cloning this repo!

- This project utilizes CMake to generate its build files.
- Targets C++20 and C99 language standards.
- You need protobuf 3.21.12 (protoc and the static libs) built by the same compiler you build this repo with. `install.ps1` takes care of that.

### Project Structure

#### Protobuf
The protobuf code is generated from the `*.proto` files found in `cci/packets/`. This done in `CMakeLists.txt` using the Protobuf CMake package.

#### cci
This is the source code for the C++ library.
It uses the PIMPL idiom to keep a consistent public interface and to avoid the need to leak internal types and implementation details.
The API exposed by this library is defined by the public versions of PIMPL classes found in *public_include/contactci.h*;
If you change any of the classes as part of the API (e.g. the `*Session` types) you'll need to ensure that *public_include/contactci.h*
is updated as well.

The primary means of utilizing this library is by creating an instance of `Session`, `HapticSession`, or `MutableHapticSession`

The *packets/* directory contains the git submodule for protobuf files describing the packets for talking to the service and devices.

#### ccic
This is the source code fo the C library. This library depends upon and wraps `cci`.

The API functions return `CciStatus` where appropriate to indicate success or errors. This enum can be found in `types/include/ccic/error.h`

#### types
This is a header-only library defining shared types used by both `cci` and `ccic`
- **types/include/ccic/lib_defs.h**
    - This defines some preprocessor macros for managing compilation using C++ vs C compilers and properly exporting or importing symbols for linkage from shared libraries (e.g. `__declspec(dllexport)` for Windows).

#### test
A collection of one-off testing executables for testing various features of `cci` and `ccic`. They are not automated tests, but CI compiles them (`build.ps1 -AllTargets`) so they don't rot.

#### CoreConductor
The C# P/Invoke wrapper around `libccic.dll`. `CoreConductor.csproj` references the `ContactCI.Maestro.libcore` package so that IDE builds get the DLLs; that package comes from GitHub Packages, which needs a [personal access token with `read:packages`](https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-nuget-registry#authenticating-to-github-packages) configured for the `ContactCI` source in `CoreConductor/nuget.config`. `build.ps1` builds with `-p:SkipLibcorePackage=true` instead, since it has just built the DLLs itself.

### Building in an IDE

#### MinGW (CLion)
Run `install.ps1` once, then:

- File -> Settings -> Build, Execution, Deployment -> Toolchains: add a MinGW toolchain pointing at `C:\msys64\mingw64`
- File -> Settings -> CMake: add `-DCMAKE_PREFIX_PATH=<repo>/protobuf` to "CMake options"
- Reset the CMake cache and reload the project

If you update MSYS2 and the build starts failing to link protobuf, re-run `install.ps1`: it notices the new gcc and rebuilds protobuf.

### CI pipeline

`.github/workflows/ci.yml` runs on pushes to `main` and `staging`, on every tag, and on pull requests.

| Job | When | What |
| --- | --- | --- |
| `build` | always | `install.ps1` + `build.ps1 -AllTargets` on `windows-latest`; uploads `artifacts/` as `packages` and `sbom/` as `sbom` |
| `security` | always | gitleaks over the full history; non-blocking |
| `sbom` | always | validates `sbom/bom.json` against CycloneDX 1.5 and checks its hashes against `artifacts/`; non-blocking |
| `staging` | push to `staging` | `X.Y.Z-staging.N` NuGet packages to GitHub Packages; keeps the newest 10 |
| `release` | release tag | GitHub Release with `build.zip` and `SHA256SUMS.txt` |
| `nuget` | release tag | GitHub Packages; nuget.org too when `RELEASE_NUGET` is `true` |
| `npm` | release tag and `RELEASE_NPM` is `true` | npmjs via trusted publishing |

The release and staging jobs publish the files `build` produced; nothing is rebuilt between testing and publishing. The version comes from `tools/resolve-version.ps1`, which you can run locally to see what a ref would produce:

```powershell
.\tools\resolve-version.ps1 -Ref refs/tags/2.4.0-beta
.\tools\resolve-version.ps1 -Ref refs/heads/staging -RunNumber 42
```

#### Repository and organisation settings

| Name | Kind | Purpose |
| --- | --- | --- |
| `SUBMODULE_APP_ID` | variable (org) | GitHub App that can read `communication`; must be installed on this repo too |
| `SUBMODULE_APP_PRIVATE_KEY` | secret (org) | that App's private key |
| `SUBMODULE_TOKEN` | secret | PAT fallback for the submodule when no App is configured |
| `RELEASE_NUGET` | variable | `true` to also publish to nuget.org |
| `NUGET_PUBLISH_KEY` | secret | nuget.org API key; required once `RELEASE_NUGET` is on |
| `RELEASE_NPM` | variable | `true` to publish to npmjs |

npm publishing uses [trusted publishing](https://docs.npmjs.com/trusted-publishers), so there is no npm token. Each of the two `@contactci` packages needs a trusted publisher on npmjs: organisation `Contact-Control-Interfaces`, repository `legacycore`, workflow `ci.yml`, no environment.

Internal npm publishing to GitHub Packages, which the GitLab pipeline did, is gone: GitHub's npm registry only hosts a scope matching the owner, and `@contactci` is not `Contact-Control-Interfaces`.

### Releases

Tag a semantic version and push it:

```bash
git tag 2.3.13
git push origin 2.3.13
```

Tag format, prereleases, tags without a release, staging prereleases, how
CoreConductor's dependency on libcore is versioned, and what to do when a
release goes wrong are all in [RELEASE.md](RELEASE.md). Security reports go
through [SECURITY.md](SECURITY.md).
