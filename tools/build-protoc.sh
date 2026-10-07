#!/usr/bin/env bash
# Rebuild protobuf/bin/protoc.exe from the static libraries in protobuf.zip.
#
# The protoc.exe inside protobuf.zip was built with GCC 13 and dynamically links the
# MinGW runtime DLLs (libstdc++-6, libgcc_s_seh-1, libwinpthread-1). Newer MSYS2
# runtimes no longer start it (STATUS_ENTRYPOINT_NOT_FOUND, exit 3221225785), so CI
# and anyone with an up-to-date MSYS2 relink it here, statically, against whatever
# toolchain is installed. The generated code and the libraries we link libcci against
# are unchanged: same protobuf 3.21.12, same libprotobuf.a from the zip.
#
# Run from an MSYS2 MINGW64 shell, anywhere, after extracting protobuf.zip at the repo root.
set -euo pipefail
cd "$(dirname "$0")/.."
if [ ! -f protobuf/lib/libprotoc.a ]; then
    echo "protobuf/ not found: extract protobuf.zip at the repo root first" >&2
    exit 1
fi
g++ -O2 -static -pthread -Iprotobuf/include tools/protoc/main.cc \
    -Lprotobuf/lib -lprotoc -lprotobuf -o protobuf/bin/protoc.exe
protobuf/bin/protoc.exe --version
