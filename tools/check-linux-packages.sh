#!/usr/bin/env bash
# =============================================================================
# check-linux-packages.sh
# -----------------------------------------------------------------------------
# Installs the .debs that build.sh --package wrote on a clean Ubuntu container
# of the same release as this host, and checks what a consumer relies on:
#
#   1. the runtime package alone gives a loadable libccic.so.<major>
#      (in the ldconfig cache, and dlopen() succeeds with its dependencies);
#   2. the dev package holds only the C API (no cci/ C++ headers, no libcci.so
#      link: INN-161), and every header it ships compiles on its own as C and
#      as C++;
#   3. with the dev package, a C program that includes <contactci.h> compiles,
#      links with -lccic and runs;
#   4. the Python package depends on exactly this build of the runtime package
#      and apt refuses to install it without that; installed, the system
#      python3 imports contactci with no PYTHONPATH, the binding loads the
#      installed libccic.so.<major> by SONAME and finds every function it
#      declares, and removing the package leaves nothing behind.
#
# No service or glove is needed: the one Session opened here is refused for
# want of a Service, which is what the check expects. Needs docker.
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
python=("$packages"/python3-contactci_*.deb)
shopt -u nullglob
[ ${#runtime[@]} -eq 1 ] && [ ${#dev[@]} -eq 1 ] && [ ${#python[@]} -eq 1 ] ||
    die "expected one runtime, one dev and one Python .deb in $packages"

docker run --rm -v "$packages:/packages:ro" \
    -e RUNTIME="/packages/$(basename "${runtime[0]}")" \
    -e DEV="/packages/$(basename "${dev[0]}")" \
    -e PYTHON="/packages/$(basename "${python[0]}")" \
    "ubuntu:$release" bash -euo pipefail -c '
        export DEBIAN_FRONTEND=noninteractive
        apt-get update -qq
        runtime_package="$(dpkg-deb -f "$RUNTIME" Package)"
        python_package="$(dpkg-deb -f "$PYTHON" Package)"

        echo "== the Python package alone: apt refuses it without the runtime package"
        wanted="$runtime_package (= $(dpkg-deb -f "$RUNTIME" Version))"
        depends="$(dpkg-deb -f "$PYTHON" Depends)"
        echo "$python_package depends on: $depends"
        grep -qF "$wanted" <<<"$depends" || { echo "$python_package does not depend on $wanted"; exit 1; }
        if apt-get install -y --no-install-recommends --simulate "$PYTHON" >/tmp/alone.log 2>&1; then
            cat /tmp/alone.log
            echo "apt would install $python_package without $runtime_package"
            exit 1
        fi
        grep -F "Depends: $runtime_package" /tmp/alone.log ||
            { cat /tmp/alone.log; echo "apt refused $python_package, but not for want of $runtime_package"; exit 1; }

        echo "== runtime package alone"
        apt-get install -y -qq --no-install-recommends "$RUNTIME" >/dev/null
        dpkg -L "$runtime_package" | grep "\.so"
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
        dev_files="$(dpkg -L "$(dpkg-deb -f "$DEV" Package)")"
        if grep -E "/include/cci(/|$)|/libcci\.so$" <<<"$dev_files"; then
            echo "the dev package ships the C++ headers or a libcci.so link: the C API is the only Linux interface (INN-161)"
            exit 1
        fi
        apt-get install -y -qq --no-install-recommends g++ >/dev/null
        for header in $(grep "^/usr/include/.*\.h$" <<<"$dev_files"); do
            echo "#include <${header#/usr/include/}>" > /tmp/header.c
            gcc -x c -fsyntax-only /tmp/header.c
            g++ -x c++ -fsyntax-only /tmp/header.c
            echo "compiles on its own as C and C++: $header"
        done
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

        echo "== with the Python package"
        # The runtime package of the same build is installed by now; python3 comes from Ubuntu.
        apt-get install -y -qq --no-install-recommends "$PYTHON" >/dev/null
        dpkg -L "$python_package" | grep -E "/dist-packages/contactci/|/doc/$python_package/"
        modules=/usr/lib/python3/dist-packages/contactci
        [ -f "$modules/py.typed" ] || { echo "$python_package ships no py.typed"; exit 1; }
        # Read from the .deb: Ubuntu container images tell dpkg not to unpack /usr/share/doc.
        # (grep -c, not -q: it reads to the end, so tar is not cut off under pipefail.)
        dpkg-deb --fsys-tarfile "$PYTHON" | tar -xO "./usr/share/doc/$python_package/EULA.md" |
            grep -c "SDK END USER LICENSE AGREEMENT" >/dev/null || { echo "$python_package ships no EULA.md"; exit 1; }
        ls "$modules"/__pycache__/*.pyc >/dev/null || { echo "$python_package was not byte-compiled at install"; exit 1; }
        RUNTIME_FILES="$(dpkg -L "$runtime_package")" CONTACTCI_SERVICE_SOCKET=/nonexistent/service.sock \
            env -u PYTHONPATH -u CONTACTCI_LIBRARY python3 - <<EOF
import os, sys
import contactci

where = os.path.dirname(contactci.__file__)
print("contactci imported from", where, "by Python", sys.version.split()[0])
assert where == "$modules", where

library = contactci.load_library()      # by SONAME through the loader; fails if a declared function is missing
mapped = sorted({line.split()[-1] for line in open("/proc/self/maps") if "/libccic.so" in line})
print("load_library() loaded", library._name, "from", mapped)
assert library._name == contactci.SONAME, library._name
assert len(mapped) == 1 and mapped[0] in os.environ["RUNTIME_FILES"].split(), mapped
assert library.cci_get_error_string(contactci.CciStatus.CCI_SUCCESS) == b"Success"

try:
    contactci.Session()
except contactci.CciError as error:
    print("Session() with no Service:", error)
    assert error.status is contactci.CciStatus.CCI_ERR_PIPE_FAILED_TO_OPEN, error.status
else:
    raise AssertionError("Session() opened with no Service")
EOF

        echo "== removing the Python package leaves nothing behind"
        apt-get remove -y -qq "$python_package" >/dev/null
        if [ -e "$modules" ]; then
            ls -R "$modules"
            echo "removing $python_package left $modules behind"
            exit 1
        fi
        echo "packages ok on ubuntu:'"$release"'"
    '
