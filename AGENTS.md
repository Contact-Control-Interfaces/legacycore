# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Native client libraries ("libcore") for talking to the Contact CI **Maestro Windows service** (haptic gloves), plus a C# P/Invoke wrapper ("CoreConductor"). Shipped as NuGet (`ContactCI.Maestro.libcore`, `ContactCI.Maestro.CoreConductor`) and NPM (`@contactci/com.contactci.libcore`, `@contactci/com.contactci.coreconductor`) packages. See `BUILDING.md` for the full build/release story.

The code is currently Windows-only (Win32 named pipes, `OpenFileMapping`, `OpenEvent`). The `feature/linux-support` branch is starting to add `#ifdef _WIN32` guards around these.

## Setup and build

```bash
git submodule update --init --recursive   # cci/packets is a private submodule (gitlab.contact.ci:sdk/libraries/communication), needs SSH keys
```

Protobuf **3.22** is required; a prebuilt copy is in `protobuf.zip` at the repo root. CI (MinGW via MSYS2) does:

```bash
# unzip protobuf.zip to ./protobuf first
cmake -DCMAKE_PREFIX_PATH=<repo>/protobuf -DCMAKE_BUILD_TYPE=Release -S . -B build/release-mingw
cmake --build build/release-mingw --target ccic -j10
```

Packaging (`.nuspec`, `package.json`) expects `build/release-mingw/libcci.dll` and `libccic.dll`. MSVC builds are disabled/unused.

C# wrapper and the HaptionReplay tool:

```bash
cd CoreConductor && dotnet build -c Release   # net8.0
```

CoreConductor pulls the native DLLs from the published `ContactCI.Maestro.libcore` NuGet package, not from a local CMake build.

## Tests

There is no test framework. `test/` holds four one-off executables (`test_cci`, `test_ccic`, plus `*_devices` variants) that link the libraries and exercise them against a running Maestro service. Build one with `cmake --build <build-dir> --target test_cci`. Running them requires the Windows service to be running.

## Architecture

Three CMake targets, all built from the root `CMakeLists.txt`:

- **`types`**: header-only INTERFACE lib in `types/include/ccic/`, shared by C and C++. `lib_defs.h` defines the export/import and `extern "C"` macros (`CCI_API_FUNC`, `CCI_API_CLASS`, keyed on `CCI_API_EXPORT` / `CCIC_API_EXPORT`). `error.h` defines `CciStatus`, and `haptic_state.h` defines the `HapticState` POD that lives in shared memory.
- **`cci`** (C++20 shared lib, `libcci`): the real implementation. Generates protobuf C++ from `cci/packets/proto/*.proto` at build time.
- **`ccic`** (C shared lib, `libccic`): a flat C API in `ccic/include/contactci.h` that wraps `cci`. Everything is in `ccic/src/exports.cpp`. It turns C++ exceptions (`contactci::Exception`) into `CciStatus` codes. This is the DLL that the C# wrapper P/Invokes.

All builds statically link libgcc/libstdc++ (`-static`).

### Runtime model (cci)

- **Control channel**: `Channel` (abstract) handles length-delimited protobuf framing (header = opcode + length) under a spinlock. `PipeChannel` implements it over the Win32 named pipe `\\.\pipe\contact-ci-service`, and `NamedPipe` wraps the handle.
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

## Releases (CI on GitLab, `.gitlab-ci.yml`)

- Pushing a semver tag creates a **public** release (NuGet + NPM, and nuget.org/npmjs too when the tag is on `main`). Use suffixes like `-beta`, `-dev`, or `-rc` for anything that isn't stable, and never use feature names in tags. CI converts `X.Y.Z-beta` to `X.Y.Z-beta.0` for NPM.
- `[SKIP CI]` / `[CI SKIP]` in a commit or tag message skips the pipeline. Non-semver tags must carry this marker.
- When you bump the major or minor version, manually update the libcore dependency range in all three places: `CoreConductor/CoreConductor/CoreConductor.csproj`, `CoreConductor/.nuspec`, and `CoreConductor/package.json`. The lower bound of the range must be a version that already exists.

## Other

- `CoreConductor/HaptionReplay`: a .NET console tool that replays, stress-tests, and analyzes haptic sessions through `MaestroSessionManager`.
- `cmake-build-*/`, `build/`, and `deploy/` are gitignored build outputs (CLion is the usual IDE).

## Agent skills

### Issue tracker

Issues live in Linear, in the **Linux SDK** project (team `DEV`), and are handled with the Linear MCP tools. See `docs/agents/issue-tracker.md`.

### Triage labels

Triage roles map to Linear statuses (Triage / Todo / Needs Human / Wont Fix), plus a `Needs Info` label. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: one `CONTEXT.md` and `docs/adr/` at the repo root. See `docs/agents/domain.md`.
