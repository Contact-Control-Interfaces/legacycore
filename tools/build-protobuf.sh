#!/usr/bin/env bash
# Build protobuf 3.21.12 (static libraries + protoc) into ./protobuf with the MinGW
# toolchain on PATH, so it always matches the compiler that builds libcci.
#
# Why not protobuf.zip: its libraries and protoc.exe were built with GCC 13 and refer to
# libstdc++ internals (std::call_once's __once_call/__once_callable TLS symbols) that
# newer libstdc++ no longer exports. With a current MSYS2, protoc.exe fails to start
# (STATUS_ENTRYPOINT_NOT_FOUND) and linking against libprotobuf.a fails with undefined
# references. Building from source with the installed toolchain avoids both for good.
#
# Run from an MSYS2 MINGW64 shell (needs gcc, cmake, ninja, curl, tar). Output is reused
# until the protobuf version or the compiler version changes.
set -euo pipefail
cd "$(dirname "$0")/.."

VERSION=3.21.12
TAG=v21.12
PREFIX="$PWD/protobuf"
SRC="$PWD/protobuf-build/src"
BUILD="$PWD/protobuf-build/build"
STAMP="$PREFIX/.built-with"
want="protobuf $VERSION gcc $(gcc -dumpfullversion)"

if [ -f "$STAMP" ] && [ "$(cat "$STAMP")" = "$want" ] && [ -x "$PREFIX/bin/protoc.exe" ]; then
    echo "protobuf already built: $want"
    "$PREFIX/bin/protoc.exe" --version
    exit 0
fi

echo "building $want"
rm -rf "$PREFIX" "$SRC" "$BUILD"
mkdir -p "$SRC"
curl -fsSL "https://github.com/protocolbuffers/protobuf/releases/download/$TAG/protobuf-cpp-$VERSION.tar.gz" \
    | tar -xz -C "$SRC" --strip-components=1

# 3.21.x keeps its CMake project under cmake/ (a top-level CMakeLists.txt arrived later)
cmake_src="$SRC"; [ -f "$SRC/CMakeLists.txt" ] || cmake_src="$SRC/cmake"
cmake -G Ninja -S "$cmake_src" -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -Dprotobuf_BUILD_TESTS=OFF \
    -Dprotobuf_BUILD_SHARED_LIBS=OFF \
    -Dprotobuf_WITH_ZLIB=OFF \
    -DCMAKE_EXE_LINKER_FLAGS=-static   # self-contained protoc.exe
cmake --build "$BUILD"
cmake --install "$BUILD"
echo "$want" > "$STAMP"
"$PREFIX/bin/protoc.exe" --version
