#!/usr/bin/env bash
# =============================================================================
# build.sh
# -----------------------------------------------------------------------------
# Linux build of libcci and libccic: the Linux counterpart of build.ps1, used
# both locally and by the build-linux job in .github/workflows/ci.yml.
#
# Builds only. Nothing is packaged for Linux until the POSIX OS primitives
# (cci/src/*_posix.cpp) are implemented.
#
# Produces build/release-linux/libcci.so and libccic.so, with protobuf linked
# in statically (tools/build-protobuf.sh builds it into ./protobuf first).
#
#     ./build.sh                    # libcci + libccic
#     ./build.sh --all-targets      # also the test/manual tools
#
# Supported on Ubuntu 22.04 and 24.04 LTS. Needs gcc, g++, cmake (3.22+), ninja,
# curl and readelf:
#     sudo apt-get install build-essential cmake ninja-build curl binutils
# =============================================================================
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$root/build/release-linux"
protobuf_dir="${PROTOBUF_DIR:-$root/protobuf}"
targets=(--target ccic)

for arg in "$@"; do
    case "$arg" in
        --all-targets) targets=() ;;
        *) echo "build.sh: unknown argument '$arg'" >&2; exit 2 ;;
    esac
done

die() { echo "build.sh: $*" >&2; exit 1; }
step() { echo; echo "== $*"; }

[ "$(uname -s)" = "Linux" ] || die "this is the Linux build; on Windows use build.ps1"
for tool in cmake ninja readelf; do
    command -v "$tool" >/dev/null || die "'$tool' not found. Install it with: sudo apt-get install build-essential cmake ninja-build curl binutils"
done

# -----------------------------------------------------------------------------
step "Checking the cci/packets submodule"
# -----------------------------------------------------------------------------
shopt -s nullglob
protos=("$root"/cci/packets/proto/*.proto)
shopt -u nullglob
[ ${#protos[@]} -gt 0 ] || die "no .proto files in cci/packets/proto. They come from the communication submodule: git submodule update --init --recursive"
echo "${#protos[@]} .proto file(s)"

# -----------------------------------------------------------------------------
step "Protobuf"
# -----------------------------------------------------------------------------
bash "$root/tools/build-protobuf.sh" "$protobuf_dir"

# -----------------------------------------------------------------------------
step "Building libcci and libccic (Release)"
# -----------------------------------------------------------------------------
# A build directory left by an IDE or an older toolchain must not leak a stale
# cache into this build. (Not cmake --fresh: that needs CMake 3.24, and
# Ubuntu 22.04 has 3.22.)
rm -rf "$build_dir/CMakeCache.txt" "$build_dir/CMakeFiles"
cmake -G Ninja -S "$root" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$protobuf_dir"
cmake --build "$build_dir" "${targets[@]}"

# -----------------------------------------------------------------------------
step "Checking what the libraries link against"
# -----------------------------------------------------------------------------
# protobuf must be inside libcci.so, not a dependency on whatever libprotobuf
# the target machine has. Only the C/C++ runtime is linked dynamically.
for lib in libcci.so libccic.so; do
    path="$build_dir/$lib"
    [ -f "$path" ] || die "expected $path was not produced"
    needed="$(readelf -d "$path" | sed -n 's/.*(NEEDED).*\[\(.*\)\]/\1/p' | tr '\n' ' ')"
    case " $needed" in
        *" libprotobuf"*) die "$lib links libprotobuf dynamically ($needed): protobuf should be linked in statically" ;;
    esac
    echo "$lib: needs $needed"
done
