# AGENTS.md

Guidance for coding agents working in this repository.

## What this is

Native client libraries ("libcore", the shipping **Core**) for talking to the Contact CI Service that owns the haptic gloves: the Windows Service today, the Linux Service (`linux-service`) on Linux. Plus a C# P/Invoke wrapper ("CoreConductor"). Shipped as NuGet (`ContactCI.Maestro.libcore`, `ContactCI.Maestro.CoreConductor`) and NPM (`@contactci/com.contactci.libcore`, `@contactci/com.contactci.coreconductor`) packages. `README.md` covers building and CI, and `RELEASE.md` covers releases.

Each OS primitive (`named_event`, `named_pipe`, `pipe_channel`, `shared_memory`) has a `*_win32.cpp` and a `*_posix.cpp` source, and CMake compiles only the current platform's. Windows uses a named pipe, `OpenFileMapping` and `OpenEvent`. Linux (DEV-6, DEV-7, DEV-17, merged 21 Oct 2026) uses a Unix socket at `/run/contactci/service.sock`, `shm_open` on the names the Service sends, and a futex sequence counter as the change signal; the contract both sides implement is `communication`'s `docs/linux-core-service-contract.md`, and the reason the Linux signal differs from the Windows pulse is `docs/adr/0001-linux-change-signal-is-a-futex-sequence-counter.md`. `CONTACTCI_SERVICE_SOCKET`, when set and not empty, replaces the socket path (ignored in setuid/setgid processes); it exists for the loopback test (DEV-19), which runs the Linux transport against a fake daemon in every Linux CI leg.

## Setup and build

```bash
git submodule update --init --recursive   # cci/packets is the private Contact-Control-Interfaces/communication repo
```

On Windows, `install.ps1` provisions MSYS2 (MINGW64 gcc, cmake, ninja, gtest), the .NET 8 SDK, nuget.exe, and protobuf 3.21.12, which `tools/build-protobuf.sh` builds from source into `./protobuf` with the current compiler. `build.ps1` then builds and packages; CI runs the same two scripts:

```powershell
.\install.ps1                 # once
.\build.ps1                   # libcci/libccic + CoreConductor, packaged into .\artifacts
.\build.ps1 -SkipPack         # compile only
```

On Linux, `./build.sh` builds protobuf and `build/release-linux/libcci.so` / `libccic.so` (no packaging); `./build.sh --tests` also builds and runs `test/unit` (needs `libgtest-dev`). The `build-linux` CI job runs `./build.sh --all-targets --tests` on Ubuntu 22.04 and 24.04 LTS, on x86_64 and arm64 (NVIDIA Jetson, JetPack 6/7): the supported Linux targets.

The underlying CMake build is:

```bash
cmake -G Ninja -DCMAKE_PREFIX_PATH=<repo>/protobuf -DCMAKE_BUILD_TYPE=Release -S . -B build/release-mingw
cmake --build build/release-mingw --target ccic
```

Packaging (`.nuspec`, `package.json`) expects `build/release-mingw/libcci.dll` and `libccic.dll`. MSVC builds are disabled/unused.

C# wrapper and the HaptionReplay tool:

```bash
cd CoreConductor && dotnet build -c Release   # net8.0
```

In an IDE build, CoreConductor pulls the native DLLs from the published `ContactCI.Maestro.libcore` NuGet package, not from a local CMake build. `build.ps1` sets `SkipLibcorePackage=true` to turn that off.

## Tests

The automated suite is GoogleTest in `test/unit`, built only with `-DCCI_BUILD_UNIT_TESTS=ON` (needs gtest). It has two executables, both registered with CTest:

- `unit_tests`: the internal classes (framing, spinlock, named events, shared memory, haptic state, errors), compiled straight from `cci/src`.
- `api_tests`: the public C++ and C APIs, linked against the real `libcci`/`libccic` and talking to a fake Windows service (`test/unit/support/fake_service.*`). No real service or glove is needed.

On Linux the same suite runs against POSIX objects instead (`test/unit/support/posix_objects.*`: shm regions and futex counters created as the Service creates them). `api_tests` there is the loopback test: a fake daemon (`test/unit/support/fake_daemon_posix.*`) listens on a Unix socket in a temporary directory, points Core at it with `CONTACTCI_SERVICE_SOCKET`, follows `communication`'s `docs/linux-core-service-contract.md`, and fails any test in which Core broke that contract on the wire. Linux-only cases are in `internal/posix_primitives_test.cpp` and `api/linux_loopback_test.cpp`; the few Windows-only cases are under `#ifdef _WIN32`. `service_fixture.h` picks the fake for the platform, so a new API test is platform-neutral unless it needs OS calls.

```powershell
.\build.ps1 -IncludeTests     # build, run the suite, then package; a failure stops the build
.\tools\run-tests.ps1         # re-run already-built tests
```

```bash
./build.sh --tests            # Linux: build, then run the suite with CTest; a failure stops the build
tools/run-tests.sh            # re-run already-built tests
```

JUnit XML goes to `reports/test-results/` (`unit_tests.xml`, `api_tests.xml`) on both platforms. The four device tests in `api_tests` are skipped unless `CCI_DEVICE_TESTS=1` is set on a machine with the service running and a glove connected.

`test/manual` holds four one-off executables (`test_cci`, `test_ccic`, `test_cci_devices`, `test_ccic_devices`) that exercise the libraries against a running Maestro service. They are compiled but never run by CI. The two `*_devices` tools use `windows.h` and are only built on Windows. Build one with `cmake --build <build-dir> --target test_cci`.

## Architecture

Three CMake targets, all built from the root `CMakeLists.txt`:

- **`types`**: header-only INTERFACE lib in `types/include/ccic/`, shared by C and C++. `lib_defs.h` defines the export/import and `extern "C"` macros (`CCI_API_FUNC`, `CCI_API_CLASS`, keyed on `CCI_API_EXPORT` / `CCIC_API_EXPORT`). `error.h` defines `CciStatus`, and `haptic_state.h` defines the `HapticState` POD that lives in shared memory.
- **`cci`** (C++20 shared lib, `libcci`): the real implementation. Generates protobuf C++ from `cci/packets/proto/*.proto` at build time.
- **`ccic`** (C shared lib, `libccic`): a flat C API in `ccic/include/contactci.h` that wraps `cci`. Everything is in `ccic/src/exports.cpp`. It turns C++ exceptions (`contactci::Exception`) into `CciStatus` codes. This is the DLL that the C# wrapper P/Invokes.

Windows builds statically link libgcc/libstdc++ (`-static`), and `build.ps1` fails if either DLL imports a MinGW runtime DLL. Linux builds link shared.

### Runtime model (cci)

- **Control channel**: `Channel` (abstract) handles length-delimited protobuf framing (header = opcode + length) under a spinlock. `PipeChannel` implements it over the Win32 named pipe `\\.\pipe\contact-ci-service` or the Unix socket `/run/contactci/service.sock`, and `NamedPipe` wraps the handle or fd (`NativeHandle` in `cci/include/cci/platform.h`). The pipe path is the only IPC name compiled into Core; every shared-memory and event name arrives in the session-init response.
- **Sessions** (`session.h/.cpp`): the hierarchy is `Session` → `HapticSession` → `MutableHapticSession`. Each constructor opens its own `PipeChannel` and sends `initialize_session(isHaptic, wantsWrite)`. The service replies with the *names* of the shared memory regions and named events to use for that session.
- **Haptic state**: `HapticStateManager` and `MutableHapticStateManager` map a named shared-memory region (`SharedMemoryManager`) that holds two consecutive `HapticState` structs (left, then right). They use a `NamedEvent` to wait for changes (global state) or to signal them (session state).
- **Client/device lists**: `EventDrivenValueMonitor<T>` (`value_monitor.h`) runs a background thread. The thread waits on a service-named event and re-queries the pipe when the event fires. Exceptions from that thread are stored and rethrown on `get_value()`.
- Public classes use **PIMPL** (`class Implementation; std::unique_ptr<Implementation>`). The subclasses each keep their own `implementation` member and chain through protected constructors that take a `unique_ptr`.

### Keeping the API surfaces in sync

The same `HapticState`, session, and device concepts appear in several places. A change to one usually needs matching changes in the others:

1. `cci/include/cci/*.h`: the internal C++ headers.
2. `cci/public_include/contactci.h`: the hand-maintained public C++ header that is installed instead of the internal headers. It includes `cci/lib_defs.h` / `cci/haptic_state.h` (install paths), so it does not compile in-tree.
3. `ccic/include/contactci.h` and `ccic/src/exports.cpp`: the C API.
4. `CoreConductor/CoreConductor/MaestroSessionManager.cs` (`DllImport("libccic")`) and `SerializableHapticState.cs`. The C# struct must match the `HapticState` field layout exactly, because it is read straight through a pointer into shared memory.

Per `BUILDING.md`: **don't add C++ APIs without a matching C# wrapper.** The C++ and C# packages are released together under the same version.

## Branches

The code is hosted on GitHub (`Contact-Control-Interfaces/legacycore`). `staging` is the integration branch, so branch features from it and target it with PRs. `main` is the stable release branch.

## Releases (GitHub Actions, `.github/workflows/ci.yml`)

See `RELEASE.md`; the rules are the dev-handbook's (`releases/versioning.md`). In short:

- `VERSION` at the repo root is the X.Y.Z `staging` is working towards. `tools/resolve-version.ps1` reads it; the `.nuspec` and `package.json` versions are placeholders that `build.ps1` overwrites at pack time. Bump `VERSION` on `staging` right after a release.
- Every push to `staging` publishes `X.Y.Z-beta.N` NuGet packages to the internal GitHub Packages feed (newest 10 kept). Pull requests and `main` publish nothing.
- Only a person tags, and only `vX.Y.Z-rc.N` on a `staging` commit (draft prerelease, npm dist-tag `next`) or `vX.Y.Z` on `main` after `git merge --ff-only` to the rc commit (draft release, `latest`, nuget.org and npmjs when `RELEASE_NUGET` / `RELEASE_NPM` are on). A tag that doesn't match `VERSION`, isn't on its branch, or is at or below the last release fails in `build` before anything is built. Tags are permanent; never use feature names in them.
- Never write a skip-CI marker (`[skip ci]` and its variants) in a commit message or a pull request body. GitHub creates no run for that push, and a release tag on that commit publishes nothing. The 3.0.3 release lost an rc number to this.
- `build.ps1` writes CoreConductor's dependency range on libcore from the release version. The range in `CoreConductor/CoreConductor/CoreConductor.csproj` only affects IDE builds.
- `cci/packets` pins a `communication` release tag on `main` (`v1.0.0`); `staging` may pin a `communication` `staging` commit while a protocol change is integrated. A protocol change usually means releasing `communication`, then this, then the Service.

## Other

- `CoreConductor/HaptionReplay`: a .NET console tool that replays, stress-tests, and analyzes haptic sessions through `MaestroSessionManager`.
- `cmake-build-*/`, `build/`, and `deploy/` are gitignored build outputs (CLion is the usual IDE).

## Agent skills

### Issue tracker

Issues live in Linear, in the **LegacyCore Lib** project (team `DEV`), and are handled with the Linear MCP tools. See `docs/agents/issue-tracker.md`.

### Triage labels

Triage roles map to Linear statuses (Triage / Todo / Needs Human / Wont Fix), plus a `Needs Info` label. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: one `CONTEXT.md` and `docs/adr/` at the repo root. See `docs/agents/domain.md`.
