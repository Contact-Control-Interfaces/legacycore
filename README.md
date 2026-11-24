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
| MSYS2 | `C:\msys64` | downloaded if absent |
| MINGW64 gcc, cmake, ninja, gtest | inside MSYS2 | `pacman -Syu --needed`, only when it is missing; gtest (GoogleTest) is for the unit tests |
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

It also writes `.\sbom\bom.json`, a CycloneDX 1.5 SBOM of those files. The SBOM covers what is linked into the DLLs (protobuf, the GCC runtime, winpthreads, the mingw-w64 CRT), the communication commit, and the hash of every artifact. In CI it is the Windows half of the release's SBOM: the `sbom` job merges it with the Linux packages' (see [Linux](#linux)).

`build.ps1` checks that both DLLs import no MinGW runtime DLLs (`libstdc++`, `libgcc`, `libwinpthread`). They are linked `-static` so that customers need nothing but the DLLs, and a missing flag would otherwise only show up on a machine without MSYS2.

### Linux

Linux support is for Ubuntu 22.04 and 24.04 LTS on x86_64 and arm64 (NVIDIA Jetson with JetPack 6 or 7): libcci and libccic build, the POSIX OS primitives (`cci/src/*_posix.cpp`: Unix socket, shared memory, futex change signal) are tested in CI against a fake daemon, and the libraries are packaged as `.deb`s and a tarball (below).

```bash
sudo apt-get install build-essential cmake ninja-build curl binutils debhelper
./build.sh                  # build/release-linux/libcci.so and libccic.so
./build.sh --all-targets    # also the test/manual tools that build on Linux
./build.sh --tests          # also build and run test/unit and python/tests (needs: sudo apt-get install libgtest-dev python3)
./build.sh --package        # also the .debs and tarball in build/packages (needs: sudo apt-get install debhelper)
tools/check-linux-packages.sh   # install and use them on a clean Ubuntu container (docker)
```

Core connects to the Service's socket at `/run/contactci/service.sock`. If the `CONTACTCI_SERVICE_SOCKET` environment variable is set and not empty, Core connects there instead; the loopback tests use it, because a CI runner can't create `/run/contactci`. Core ignores it in a setuid or setgid process.

`build.sh` builds protobuf 3.21.12 into `./protobuf` with `tools/build-protobuf.sh` (position-independent, so it links into `libcci.so`), then checks that neither library depends on `libprotobuf`. The libraries carry the `VERSION` file's version and its major version as SONAME (`libccic.so.3`).

`--package` installs the build (`cmake --install`) and packages it with `debian/` and `dpkg-buildpackage`, for the host's Ubuntu release and architecture:

| File | Holds |
| --- | --- |
| `libcontactci-core3_<version>_<distro>_<arch>.deb` | `/usr/lib/libcci.so.3*` and `/usr/lib/libccic.so.3*` |
| `libcontactci-core-dev_<version>_<distro>_<arch>.deb` | `/usr/include/contactci.h` (the C API), `/usr/include/ccic/`, `/usr/include/contactci/`, and the `libccic.so` link |
| `python3-contactci_<version>_<distro>_<arch>.deb` | the [Python binding](#python-binding): `/usr/lib/python3/dist-packages/contactci/`, and `/usr/share/doc/python3-contactci/EULA.md`. Depends on `python3 (>= 3.10)` and on the `libcontactci-core3` of the same build |
| `contactci-core_<version>_<distro>_<arch>.tar.gz` | the libraries and headers as a tree under `contactci-core-<version>/{lib,include}`, for hosts without dpkg |

`<version>` is `$CCI_VERSION` (CI sets it from `tools/resolve-version.ps1`), or `<VERSION>-local` for a local build; `<distro>` is e.g. `ubuntu22.04`. Inside the `.deb`, a prerelease is `3.0.3~rc.1` so dpkg sorts it below `3.0.3`. To use them:

```bash
sudo apt install ./libcontactci-core3_3.0.3_ubuntu22.04_amd64.deb ./libcontactci-core-dev_3.0.3_ubuntu22.04_amd64.deb
gcc app.c -lccic            # #include <contactci.h>
```

`tools/write-linux-sbom.py [build/packages] [sbom/bom-linux.json]` writes a CycloneDX 1.5 SBOM fragment for the four files: each one's hash, the `libcci.so.3.*` and `libccic.so.3.*` the runtime `.deb` and the tarball ship (the `.deb` carries stripped copies, the tarball unstripped ones, so their hashes differ), protobuf and the communication commit linked into `libcci`, and the `.deb`s' `Depends`. Every `build-linux` leg writes one, and the `sbom` job merges them with the Windows SBOM using `tools/merge-sbom.py`, and `tools/check-sbom.py` checks the result against every released file. It needs `dpkg-deb`, which `--package` needs anyway. `python3 -m unittest discover -s test/sbom -v` tests all three on fixture packages, including each way the gate must fail; the `sbom` job runs it first.

The C API is the only supported way to build against Core on Linux. The C++ headers in `cci/include/cci/` include protobuf-generated headers (`channel.h` includes `common.pb.h`) that would tie a consumer to the protobuf Core was built with, so the packages don't ship them or a `libcci.so` link; `libcci.so.3` is only there because `libccic` needs it at runtime.

### Python binding

`python/contactci` is a Python binding over the C API: pure `ctypes`, standard library only, no build step, for Python 3.10 and later (the system `python3` on Ubuntu 22.04 and 24.04). It ships as the `python3-contactci` `.deb`, built and released with Core's own packages and carrying Core's version (INN-298); there is no wheel and nothing on PyPI. Install it with the runtime package of the same release, from the same directory:

```bash
sudo apt install ./python3-contactci_3.0.4_ubuntu22.04_amd64.deb ./libcontactci-core3_3.0.4_ubuntu22.04_amd64.deb
python3 -c "import contactci; contactci.load_library()"     # the system python3, no PYTHONPATH
```

The `.deb` depends on exactly its own build of `libcontactci-core3`, so apt refuses it alone unless that runtime package is already installed, and the two are upgraded together.

To work on the binding, or to use it from a checkout, point Python at the source instead:

```bash
export PYTHONPATH=/path/to/legacycore/python        # or, in a virtual environment: pip install /path/to/legacycore/python
```

It loads `libccic.so.3` by SONAME through the dynamic loader, so it needs the runtime package (`libcontactci-core3`, above) and no `-dev` package or headers. To use a library somewhere else, such as a local build, set `CONTACTCI_LIBRARY` to its path (`build/release-linux/libccic.so` for a local build) or call `contactci.load_library(path)` before opening a Session.

```python
import time
import contactci

with contactci.MutableHapticSession() as session:    # CciError if the Service isn't running
    print(session.service_version, session.right_device())
    command = contactci.HapticState()                 # 72 bytes, everything off
    command.indexVibrationEffect = 1                  # a vibration effect code
    session.right.write(command)                      # into this Session's haptic state
    session.signal_changed()                          # the Service applies it now
    time.sleep(0.5)
    session.right.clear()                             # everything off again
    session.signal_changed()
```

`Session` gives the device and client lists and the Service's version, `HapticSession` also reads the Global haptic state (`global_left`, `global_right`, `wait_global_changed`), and `MutableHapticSession` also writes its own (`left`, `right`, `signal_changed`). A call into Core that fails raises `contactci.CciError`, which carries the `CciStatus` as `status` and Core's text for it as `message`.

Lifetime: `session.left`, `session.right`, `session.global_left` and `session.global_right` are views of memory shared with the Service, not copies. Reading or assigning a field reads or writes that memory directly, and it is unmapped when the Session closes. A view is valid only while its Session is open; after `close()`, or after the `with` block, every access raises `contactci.SessionClosedError` rather than touching unmapped memory. Take a copy with `view.read()` to keep a state past the Session. The Global haptic state is read-only. Raw pointers from `contactci.load_library()` have no such check: don't keep one past `cci_close_session`.

`contactci.HapticState` mirrors `ContactCiHapticState` field for field, so the vibration amplitude fields are `float`, as the header declares them. The Windows Service and the C# wrapper read the same four bytes as a `uint32`, and which is right is undecided (INN-14); until it is, leave them at zero and use the vibration effect fields. `python/tests/test_layout.py` compiles the header and fails if the Python struct stops matching it.

### Tests

```powershell
.\build.ps1 -IncludeTests          # build, then run test\unit before packaging
.\tools\run-tests.ps1              # re-run the already-built tests
```

On Linux:

```bash
./build.sh --tests                 # build, then run test/unit and python/tests with CTest
tools/run-tests.sh                 # re-run the already-built tests
```

The results are JUnit XML in `reports/test-results` (`unit_tests.xml`, `api_tests.xml`, and on Linux `python_tests.xml`), and a failing test stops the build. The tests need no service or glove: the API tests talk to a fake service on Windows and a fake daemon on Linux. The four device tests need real hardware, and are reported as skipped unless you run them on a machine with the service running and a glove connected, with `CCI_DEVICE_TESTS=1` set.

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
The API exposed by this library is defined by the public versions of PIMPL classes found in `public_include/contactci.h`;
If you change any of the classes as part of the API (e.g. the `*Session` types) you'll need to ensure that `public_include/contactci.h`
is updated as well.

The primary means of utilizing this library is by creating an instance of `Session`, `HapticSession`, or `MutableHapticSession`.

The `packets/` directory contains the git submodule for protobuf files describing the packets for talking to the service and devices.

#### ccic
This is the source code fo the C library. This library depends upon and wraps `cci`.

The API functions return `CciStatus` where appropriate to indicate success or errors. This enum can be found in `types/include/ccic/error.h`

#### types
This is a header-only library defining shared types used by both `cci` and `ccic`
- **types/include/ccic/lib_defs.h**
    - This defines some preprocessor macros for managing compilation using C++ vs C compilers and properly exporting or importing symbols for linkage from shared libraries (e.g. `__declspec(dllexport)` for Windows).

#### test
All tests, in two folders:

- **test/unit** - the automated suite (GoogleTest), built with `-DCCI_BUILD_UNIT_TESTS=ON`:
    - `unit_tests` covers the internal classes, compiled from `cci/src`: message framing, the spinlock, named events, shared memory, the `HapticState` layout and the error strings. On Linux, `internal/posix_primitives_test.cpp` adds the futex counter and shm cases.
    - `api_tests` covers the public C++ and C APIs, linked against the real `libcci.dll` / `libccic.dll`. A fake Windows service (`support/fake_service.*`) owns `\\.\pipe\contact-ci-service` and answers the protocol.
    - On Linux, `api_tests` is the loopback test: the same API tests, plus `api/linux_loopback_test.cpp`, run against the real `libcci.so` / `libccic.so` and a fake daemon (`support/fake_daemon_posix.*`). The daemon follows `communication`'s `docs/linux-core-service-contract.md`: it listens on a Unix socket in a temporary directory and points Core at it with `CONTACTCI_SERVICE_SOCKET`, answers session init with protocol version 1 and the global shm and counter names, passes a write Session's own region and counter as sealed memfds with `SCM_RIGHTS`, creates the 144-byte regions and futex counters (`support/posix_objects.*`), and fails the test if Core breaks the contract on the wire.
    - `api/device_test.cpp` holds the hardware tests, skipped without a device.
    - On Linux, `fake_daemon` is the same fake daemon as a program (`support/fake_daemon_main.cpp`), driven over its standard input and output by tests that are not C++: the Python binding's.
- **python/tests** - the Python binding's `unittest` suite, run by CTest on Linux with the system `python3`: the `ctypes` layout against the compiled headers, the built `libccic.so` (it loads, every function resolves, opening a Session with no Service raises `CCI_ERR_PIPE_FAILED_TO_OPEN`), the binding end to end against `fake_daemon`, and its packaging (`pyproject.toml` carries the `VERSION` file's version; the `.deb` depends on the library and the Python the binding is written for).
- **test/manual** - one-off tools that drive a real service and glove (`test_cci`, `test_ccic`, `test_cci_devices`, `test_ccic_devices`). They loop forever and check nothing, so CI only compiles them; the device tests are their automated counterparts. `test_cci_devices` and `test_ccic_devices` use `windows.h` and are only built on Windows.

#### python
The Python binding over `libccic.so` (see [Python binding](#python-binding)). `contactci/_abi.py` holds the `ctypes` mirrors of the C declarations and the loader, `contactci/_session.py` the Session classes. `EULA.md` there is a link to the one at the repository root: the Contact CI SDK EULA, copied unchanged from the dev-handbook (`licensing/templates/sdk-eula.md`), which the `.deb` installs under `/usr/share/doc/python3-contactci/`. `build.sh --package` packages `contactci/` as it is into the `python3-contactci` `.deb` (`debian/python3-contactci.*`); `pyproject.toml` only makes the directory installable by path for development, and its `version` is the `VERSION` file's (`tests/test_packaging.py` fails until they agree).

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

`.github/workflows/ci.yml` runs on pushes to `main` and `staging`, on `v*` tags, and on pull requests.

One straight line: `build` → `test` → `security` → `sbom` → `release`. Each stage starts only if the previous one passed. `build-linux` runs alongside it, and `sbom` also waits for it.

| Job | When | What |
| --- | --- | --- |
| `build` | always | install, build, and package artifacts |
| `build-linux` | always, in parallel | `./build.sh --all-targets --tests`, then `./build.sh --package` and `tools/check-linux-packages.sh`, on Ubuntu 22.04 and 24.04, x86_64 and arm64: builds, runs `test/unit` including the loopback test, packages; publishes a "Linux tests (<os>)" check, uploads the JUnit XML as `linux-test-results-<os>`, `libcci.so` / `libccic.so` as `linux-libraries-<os>` the `.deb`s and tarball as `linux-packages-<os>`, and their SBOM fragment (`tools/write-linux-sbom.py`) as `sbom-linux-<os>` |
| `test` | after `build` | run tests and publishes the "Unit tests" check and the JUnit XML as `test-results` |
| `security` | after `test` | gitleaks over the full history; a finding stops the pipeline |
| `sbom` | after `security` and every `build-linux` leg | merges `build`'s SBOM (`sbom-windows`) and every leg's fragment into one `bom.json` (`tools/merge-sbom.py`), validates it against CycloneDX 1.5, checks that it lists every file in `packages` and `linux-packages-*` with its SHA-256, and uploads it as `sbom`. A file missing from it, or with another hash, stops the pipeline |
| `release` | after `sbom`, on a release tag or a push to `staging` | **tag**: GitHub Release with `build.zip`, the Linux `.deb`s and tarballs, and `SHA256SUMS.txt`, then NuGet to GitHub Packages (and nuget.org when `RELEASE_NUGET` is `true`), then npm via trusted publishing (when `RELEASE_NPM` is `true`). **`staging`**: `X.Y.Z-beta.N` NuGet packages to GitHub Packages. Skipped for pull requests and `main` |

The release job publishes the files `build` and `build-linux` produced and nothing is rebuilt between testing and publishing. The version comes from `tools/resolve-version.ps1`, which you can run locally to see what a ref would produce:

```powershell
.\tools\resolve-version.ps1 -Ref refs/tags/v3.0.3-rc.2
.\tools\resolve-version.ps1 -Ref refs/heads/staging -RunNumber 42
```

#### Repository and organisation settings

| Name | Kind | Purpose |
| --- | --- | --- |
| `SUBMODULE_APP_ID` | variable (org) | GitHub App that can read `communication`; must be installed on this repo too |
| `SUBMODULE_APP_PRIVATE_KEY` | secret (org) | that App's private key |
| `SUBMODULE_TOKEN` | secret | PAT fallback for the submodule when no App is configured |
| `RELEASE_NUGET` | variable | `true` to also publish to nuget.org |
| `NUGET_USER` | variable | nuget.org profile name (not email) that owns the trusted publishing policy, if `RELEASE_NUGET` is on |
| `RELEASE_NPM` | variable | `true` to publish to npmjs |

nuget.org publishing uses [trusted publishing](https://learn.microsoft.com/nuget/nuget-org/trusted-publishing), so there is no NuGet API key. The policy on nuget.org needs repository owner `Contact-Control-Interfaces`, repository `legacycore`, workflow file `ci.yml`, and no environment.

npm publishing uses [trusted publishing](https://docs.npmjs.com/trusted-publishers), so there is no npm token. Each of the two `@contactci` packages needs a trusted publisher on npmjs: organisation `Contact-Control-Interfaces`, repository `legacycore`, workflow `ci.yml`, no environment.

Internal npm publishing to GitHub Packages, which the GitLab pipeline did, is gone: GitHub's npm registry only hosts a scope matching the owner, and `@contactci` is not `Contact-Control-Interfaces`.

### Releases

Tag a semantic version and push it:

```bash
git tag -a v3.0.3-rc.2 -m "3.0.3-rc.2" origin/staging
git push origin v3.0.3-rc.2
```

Tag format, prereleases, tags without a release, staging prereleases, how
CoreConductor's dependency on libcore is versioned, and what to do when a
release goes wrong are all in [RELEASE.md](RELEASE.md). Security reports go
through [SECURITY.md](SECURITY.md).
