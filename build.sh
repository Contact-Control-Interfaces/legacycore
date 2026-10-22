#!/usr/bin/env bash
# =============================================================================
# build.sh
# -----------------------------------------------------------------------------
# Linux build of libcci and libccic: the Linux counterpart of build.ps1, used
# both locally and by the build-linux job in .github/workflows/ci.yml.
#
# Produces build/release-linux/libcci.so and libccic.so, with protobuf linked
# in statically (tools/build-protobuf.sh builds it into ./protobuf first).
#
#     ./build.sh                    # libcci + libccic
#     ./build.sh --all-targets      # also the test/manual tools
#     ./build.sh --tests            # also build and run test/unit (tools/run-tests.sh)
#     ./build.sh --package          # then package them into build/packages
#
# --package writes, for this host's Ubuntu release and architecture:
#     libcontactci-core3_<version>_<distro>_<arch>.deb      the two libraries
#     libcontactci-core-dev_<version>_<distro>_<arch>.deb   headers, .so links
#     contactci-core_<version>_<distro>_<arch>.tar.gz       the same install tree
# e.g. libcontactci-core3_3.0.3-rc.1_ubuntu22.04_amd64.deb. <version> is
# $CCI_VERSION (CI passes tools/resolve-version.ps1's), else <VERSION>-local.
# Inside the .deb a prerelease is 3.0.3~rc.1, which dpkg sorts below 3.0.3.
# It needs debhelper too: sudo apt-get install debhelper
#
# Supported on Ubuntu 22.04 and 24.04 LTS. Needs gcc, g++, cmake (3.22+), ninja,
# curl and readelf:
#     sudo apt-get install build-essential cmake ninja-build curl binutils
# and for --tests, GoogleTest:
#     sudo apt-get install libgtest-dev
# =============================================================================
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$root/build/release-linux"
protobuf_dir="${PROTOBUF_DIR:-$root/protobuf}"
targets=(--target ccic)
tests=OFF
package=false

for arg in "$@"; do
    case "$arg" in
        --all-targets) targets=() ;;
        --tests) tests=ON ;;
        --package) package=true ;;
        *) echo "build.sh: unknown argument '$arg'" >&2; exit 2 ;;
    esac
done
if [ "$tests" = ON ] && [ ${#targets[@]} -gt 0 ]; then
    targets+=(unit_tests api_tests)
fi

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
    -DCMAKE_PREFIX_PATH="$protobuf_dir" \
    -DCCI_BUILD_UNIT_TESTS="$tests"
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

# -----------------------------------------------------------------------------
if [ "$tests" = ON ]; then
    step "Running the tests (test/unit)"
    # api_tests load the libcci.so and libccic.so built above, against a fake daemon.
    bash "$root/tools/run-tests.sh" "$build_dir" "$root/reports/test-results" ||
        die "tests failed; see the output above. JUnit reports: reports/test-results"
fi
$package || exit 0

# -----------------------------------------------------------------------------
step "Packaging"
# -----------------------------------------------------------------------------
command -v dpkg-buildpackage >/dev/null && command -v dh >/dev/null ||
    die "--package needs dpkg-buildpackage and debhelper: sudo apt-get install debhelper"

declared="$(tr -d '[:space:]' < "$root/VERSION")"
version="${CCI_VERSION:-$declared-local}"
[[ "$version" =~ ^([0-9]+)\.[0-9]+\.[0-9]+(-[0-9A-Za-z.]+)?$ ]] ||
    die "package version '$version' is not X.Y.Z or X.Y.Z-<prerelease>"
major="${BASH_REMATCH[1]}"
[ "${version%%-*}" = "$declared" ] || die "package version $version does not match VERSION ($declared)"
deb_version="${version/-/\~}"               # 3.0.3-rc.1 -> 3.0.3~rc.1

runtime_pkg="libcontactci-core$major"
grep -qx "Package: $runtime_pkg" "$root/debian/control" ||
    die "debian/control has no $runtime_pkg package: rename the runtime package to match SONAME major version $major"
soname="$(readelf -d "$build_dir/libccic.so" | sed -n 's/.*(SONAME).*\[\(.*\)\]/\1/p')"
[ "$soname" = "libccic.so.$major" ] || die "libccic.so has SONAME '$soname', expected libccic.so.$major"

distro="$(. /etc/os-release && echo "${ID}${VERSION_ID}")"                 # ubuntu22.04
codename="$(. /etc/os-release && echo "${VERSION_CODENAME:-unstable}")"    # jammy
arch="$(dpkg --print-architecture)"                                         # amd64, arm64
out="$root/build/packages"
rm -rf "$out"
mkdir -p "$out"

# Timestamps inside the packages come from the commit, so rebuilding a commit
# gives the same file contents.
SOURCE_DATE_EPOCH="$(git -C "$root" log -1 --format=%ct 2>/dev/null || date +%s)"
export SOURCE_DATE_EPOCH
commit="$(git -C "$root" rev-parse --short HEAD 2>/dev/null || echo unknown)"

# The tarball: the install tree under contactci-core-<version>/.
stage="$build_dir/package-stage"
tree="contactci-core-$version"
rm -rf "$stage"
cmake --install "$build_dir" --prefix "$stage/$tree"
tar -C "$stage" --sort=name --mtime="@$SOURCE_DATE_EPOCH" --owner=0 --group=0 --numeric-owner -cf - "$tree" |
    gzip -9n > "$out/contactci-core_${version}_${distro}_${arch}.tar.gz"

# The .debs. debian/changelog carries only this build's version and is not tracked.
cat > "$root/debian/changelog" <<EOF
contactci-core ($deb_version) $codename; urgency=medium

  * legacycore $version, commit $commit.

 -- Contact CI Support <support@contactci.co>  $(date -R -u -d "@$SOURCE_DATE_EPOCH")
EOF
(cd "$root" && dpkg-buildpackage -b -us -uc)

# dpkg-buildpackage writes next to the source tree. The file names get the
# distro, so the four CI legs do not collide on a release, and the SemVer
# version that names everything else in the release (the tag, build.zip, NuGet)
# rather than the Debian one with its "~".
parent="$(dirname "$root")"
for pkg in "$runtime_pkg" libcontactci-core-dev; do
    mv "$parent/${pkg}_${deb_version}_${arch}.deb" "$out/${pkg}_${version}_${distro}_${arch}.deb"
done
rm -f "$parent/contactci-core_${deb_version}_${arch}.buildinfo" "$parent/contactci-core_${deb_version}_${arch}.changes"

echo
ls -l "$out"
