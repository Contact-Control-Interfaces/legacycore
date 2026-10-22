#!/usr/bin/env bash
# =============================================================================
# check-linux-packages.sh
# -----------------------------------------------------------------------------
# Installs the .debs that build.sh --package wrote on a clean Ubuntu container
# of the same release as this host, and checks what a consumer relies on:
#
#   1. the runtime package alone gives a loadable libccic.so.<major>
#      (in the ldconfig cache, and dlopen() succeeds with its dependencies);
#   2. with the dev package, a C program that includes <contactci.h> compiles,
#      links with -lccic and runs.
#
# No service or glove is needed: nothing here opens a session. Needs docker.
#
#     tools/check-linux-packages.sh [packages-dir]     # default build/packages
# =============================================================================
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
packages="$(cd "${1:-$root/build/packages}" && pwd)"
die() { echo "check-linux-packages.sh: $*" >&2; exit 1; }

command -v docker >/dev/null || die "docker not found"
release="$(. /etc/os-release && echo "$VERSION_ID")"
shopt -s nullglob
runtime=("$packages"/libcontactci-core[0-9]*_*.deb)
dev=("$packages"/libcontactci-core-dev_*.deb)
shopt -u nullglob
[ ${#runtime[@]} -eq 1 ] && [ ${#dev[@]} -eq 1 ] ||
    die "expected one runtime and one dev .deb in $packages"

docker run --rm -v "$packages:/packages:ro" \
    -e RUNTIME="/packages/$(basename "${runtime[0]}")" \
    -e DEV="/packages/$(basename "${dev[0]}")" \
    "ubuntu:$release" bash -euo pipefail -c '
        export DEBIAN_FRONTEND=noninteractive
        apt-get update -qq

        echo "== runtime package alone"
        apt-get install -y -qq --no-install-recommends "$RUNTIME" >/dev/null
        dpkg -L "$(dpkg-deb -f "$RUNTIME" Package)" | grep "\.so"
        ldconfig -p | grep -E "libcc(i|ic)\.so\." || { echo "libcci/libccic are not in the ldconfig cache"; exit 1; }
        soname="$(ldconfig -p | sed -n "s/^[[:space:]]*\(libccic\.so\.[0-9]*\) .*/\1/p" | head -n 1)"
        echo "loading $soname"
        apt-get install -y -qq --no-install-recommends gcc libc6-dev >/dev/null
        cat > /tmp/load.c <<EOF
#include <dlfcn.h>
#include <stdio.h>
int main(void) {
    void *h = dlopen("$soname", RTLD_NOW);
    if (!h) { fprintf(stderr, "%s\n", dlerror()); return 1; }
    if (!dlsym(h, "cci_get_error_string")) { fprintf(stderr, "cci_get_error_string missing\n"); return 1; }
    puts("dlopen ok");
    return 0;
}
EOF
        gcc /tmp/load.c -o /tmp/load -ldl
        /tmp/load

        echo "== with the dev package"
        apt-get install -y -qq --no-install-recommends "$DEV" >/dev/null
        cat > /tmp/use.c <<EOF
#include <contactci.h>
#include <stdio.h>
int main(void) {
    printf("cci_get_error_string(CCI_SUCCESS) = %s\n", cci_get_error_string(CCI_SUCCESS));
    return 0;
}
EOF
        gcc /tmp/use.c -o /tmp/use -lccic
        /tmp/use
        echo "packages ok on ubuntu:'"$release"'"
    '
