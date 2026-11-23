#!/usr/bin/env python3
"""
write-linux-sbom.py
---------------------------------------------------------------
Writes a CycloneDX 1.5 SBOM fragment for the Linux packages that
`build.sh --package` put in build/packages: the Linux counterpart of
tools/write-sbom.ps1, for one Ubuntu release and architecture.

The `build-linux` CI job runs this on every leg. The `sbom` job then
merges the Windows SBOM and every Linux fragment into one bom.json
(tools/merge-sbom.py) and checks every released file against it.

What it describes:

  packages        the runtime .deb, the dev .deb, the Python .deb and the
                  tarball, each with its SHA-256 and a legacycore:artifact
                  property naming the file, which the sbom job checks the
                  file against
  libraries       libcci.so.<v> and libccic.so.<v> as each package ships
                  them: the .debs carry stripped copies (dh_strip), the
                  tarball the unstripped ones, so each has its own hash
  protobuf        version, tag and source SHA-256 from tools/build-protobuf.sh;
                  build.sh checks it is linked into libcci.so statically
  communication   the checked-out commit of cci/packets: the protocol code
                  generated into libcci and the contactci/haptic_state.h the
                  dev package and the tarball ship

The Python package (python3-contactci) holds the binding's own modules and
nothing third-party: it is described as one component that depends on the
runtime package, whose libccic it loads.

The C and C++ runtime (libc6, libstdc++6, libgcc-s1) are linked
dynamically, so they aren't bundled; the .debs' Depends field is recorded
as the legacycore:deb-depends property instead.

Needs Python 3.8+ and dpkg-deb, which build.sh --package needs anyway:
    tools/write-linux-sbom.py [build/packages] [sbom/bom-linux.json]
The version is $CCI_VERSION, else <VERSION>-local, as in build.sh.
"""

import hashlib
import io
import json
import os
import re
import subprocess
import sys
import tarfile
import uuid
from datetime import datetime, timezone

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO_URL = "https://github.com/Contact-Control-Interfaces/legacycore"
EULA = [{"license": {"name": "Contact CI SDK EULA", "url": "https://contact.ci/pages/sdk-eula"}}]
INTERNAL = [{"license": {"name": "Proprietary, Contact CI"}}]

# <package>_<semver>_<distro>_<arch>.deb and contactci-core_<semver>_<distro>_<arch>.tar.gz
DEV_PACKAGE = "libcontactci-core-dev"
PYTHON_PACKAGE = "python3-contactci"
DEB_NAME = re.compile(r"^(?P<package>libcontactci-core(?:\d+|-dev)|python3-contactci)_(?P<version>[^_]+)_(?P<distro>[a-z]+\d+\.\d+)_(?P<arch>[a-z0-9]+)\.deb$")
TAR_NAME = re.compile(r"^contactci-core_(?P<version>[^_]+)_(?P<distro>[a-z]+\d+\.\d+)_(?P<arch>[a-z0-9]+)\.tar\.gz$")
LIBRARY = re.compile(r"(?:^|/)(?P<name>libccic?)\.so\.\d+\.\d+\.\d+$")


def die(message):
    print(f"write-linux-sbom.py: {message}", file=sys.stderr)
    sys.exit(1)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def git_commit(directory):
    try:
        out = subprocess.run(["git", "-C", directory, "rev-parse", "HEAD"],
                             capture_output=True, text=True, check=True)
        return out.stdout.strip() or None
    except (OSError, subprocess.CalledProcessError):
        return None


def expected_version():
    with open(os.path.join(ROOT, "VERSION")) as f:
        declared = "".join(f.read().split())
    return os.environ.get("CCI_VERSION") or f"{declared}-local"


def deb_field(path, field):
    out = subprocess.run(["dpkg-deb", "--field", path, field], capture_output=True, text=True, check=True)
    return " ".join(out.stdout.split())


def deb_tar(path):
    # dpkg-deb decompresses data.tar.zst itself; Python's tarfile can't before 3.14.
    out = subprocess.run(["dpkg-deb", "--fsys-tarfile", path], capture_output=True, check=True)
    return tarfile.open(fileobj=io.BytesIO(out.stdout), mode="r:")


def shipped_libraries(tar, strip_prefix=""):
    """(path inside the package, SHA-256) of each regular libcci/libccic file; links are skipped."""
    found = []
    for member in tar.getmembers():
        path = member.name[2:] if member.name.startswith("./") else member.name
        if strip_prefix and path.startswith(strip_prefix):
            path = path[len(strip_prefix):]
        if member.isfile() and LIBRARY.search(path):
            found.append((path, sha256(tar.extractfile(member).read())))
    return sorted(found)


def ships_headers(tar):
    return any(m.isfile() and "include/" in m.name for m in tar.getmembers())


def protobuf_component():
    with open(os.path.join(ROOT, "tools", "build-protobuf.sh")) as f:
        script = f.read()
    values = {}
    for key in ("PROTOBUF_VERSION", "PROTOBUF_TAG", "PROTOBUF_SHA256"):
        match = re.search(rf'^{key}="([^"]+)"', script, re.M)
        if not match:
            die(f"{key} not found in tools/build-protobuf.sh")
        values[key] = match.group(1)
    purl = f"pkg:github/protocolbuffers/protobuf@{values['PROTOBUF_TAG']}"
    return {
        "type": "library",
        "bom-ref": purl,
        "group": "protocolbuffers",
        "name": "protobuf",
        "version": values["PROTOBUF_VERSION"],
        "purl": purl,
        "licenses": [{"license": {"id": "BSD-3-Clause"}}],
        "externalReferences": [{
            "type": "distribution",
            "url": f"https://github.com/protocolbuffers/protobuf/releases/download/{values['PROTOBUF_TAG']}/protobuf-cpp-{values['PROTOBUF_VERSION']}.tar.gz",
            "hashes": [{"alg": "SHA-256", "content": values["PROTOBUF_SHA256"]}],
        }],
    }


def submodule_commit(path):
    """The commit checked out in a submodule, or None if it isn't checked out.
    (Not `git -C <path> rev-parse`: in an uninitialised submodule that finds
    the parent repository and returns its commit.)"""
    try:
        out = subprocess.run(["git", "-C", ROOT, "submodule", "status", "--", path],
                             capture_output=True, text=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError):
        return None
    match = re.match(r"^[ +]([0-9a-f]{40}) ", out)
    return match.group(1) if match else None


def communication_component():
    commit = submodule_commit("cci/packets")
    component = {
        "type": "data",
        "bom-ref": "communication",
        "name": "communication",
        "version": commit or "unknown",
        "description": "Protocol definitions (cci/packets), generated into libcci",
        "licenses": INTERNAL,
    }
    if commit:
        component["purl"] = f"pkg:github/Contact-Control-Interfaces/communication@{commit}"
    return component


def library_components(artifact, libraries, version, protobuf_ref):
    """A nested component per shipped library, and the dependencies between them."""
    components, refs = [], {}
    for path, digest in libraries:
        name = LIBRARY.search(path).group("name")
        ref = f"file:{artifact}#{path}"
        refs[name] = ref
        components.append({
            "type": "file",
            "bom-ref": ref,
            "name": path,
            "version": version,
            "hashes": [{"alg": "SHA-256", "content": digest}],
            "licenses": EULA,
        })
    if set(refs) != {"libcci", "libccic"}:
        die(f"{artifact} should ship libcci.so.<version> and libccic.so.<version>, found {[p for p, _ in libraries] or 'neither'}")
    dependencies = [
        {"ref": refs["libcci"], "dependsOn": [protobuf_ref, "communication"]},
        {"ref": refs["libccic"], "dependsOn": [refs["libcci"]]},
    ]
    return components, dependencies, list(refs.values())


def artifact_component(path, kind, name, version, purl, properties, nested):
    artifact = os.path.basename(path)
    component = {
        "type": kind,
        "bom-ref": purl or f"file:{artifact}",
        "name": name,
        "version": version,
        "hashes": [{"alg": "SHA-256", "content": sha256_file(path)}],
        "licenses": EULA,
        # The sbom job checks every hash against the file named here.
        "properties": [{"name": "legacycore:artifact", "value": artifact}] + properties,
    }
    if purl:
        component["purl"] = purl
    if nested:
        component["components"] = nested
    return component


def main(argv):
    packages_dir = argv[1] if len(argv) > 1 else os.path.join(ROOT, "build", "packages")
    output_file = argv[2] if len(argv) > 2 else os.path.join(ROOT, "sbom", "bom-linux.json")
    version = expected_version()
    protobuf = protobuf_component()

    files = sorted(os.listdir(packages_dir)) if os.path.isdir(packages_dir) else []
    if not files:
        die(f"no packages in {packages_dir} - run ./build.sh --package first")

    # Every file in the directory has to be one this describes, or the sbom
    # job would find a released file with no SBOM entry.
    debs, tarballs, platforms = {}, [], set()
    for name in files:
        match = DEB_NAME.match(name) or TAR_NAME.match(name)
        if not match:
            die(f"{name} in {packages_dir} is not a package build.sh --package writes")
        if match["version"] != version:
            die(f"{name} is version {match['version']}, expected {version}")
        platforms.add((match["distro"], match["arch"]))
        if name.endswith(".deb"):
            debs[match["package"]] = os.path.join(packages_dir, name)
        else:
            tarballs.append(os.path.join(packages_dir, name))
    if len(platforms) != 1:
        die(f"{packages_dir} mixes platforms: {sorted(platforms)}")
    (distro, arch), = platforms
    platform = f"{distro}_{arch}"
    runtime = [p for p in debs if p not in (DEV_PACKAGE, PYTHON_PACKAGE)]
    if len(runtime) != 1 or DEV_PACKAGE not in debs or PYTHON_PACKAGE not in debs or len(tarballs) != 1:
        die(f"expected a runtime .deb, a dev .deb, a Python .deb and a tarball in {packages_dir}, found {files}")
    runtime = runtime[0]

    # distro=ubuntu-22.04 as purl's deb type writes it. The namespace is the
    # vendor: these come from Contact CI, not from the Ubuntu archive.
    purl_distro = re.sub(r"^([a-z]+)", r"\1-", distro)
    properties = [{"name": "legacycore:platform", "value": platform}]
    components, dependencies, package_refs = [], [], []

    for package in (runtime, DEV_PACKAGE, PYTHON_PACKAGE):
        path = debs[package]
        deb_version = deb_field(path, "Version")
        if deb_version != version.replace("-", "~", 1):
            die(f"{os.path.basename(path)} has Version {deb_version}, expected {version.replace('-', '~', 1)}")
        if deb_field(path, "Architecture") != arch:
            die(f"{os.path.basename(path)} is not built for {arch}")
        purl = f"pkg:deb/contactci/{package}@{deb_version}?arch={arch}&distro={purl_distro}"
        tar = deb_tar(path)
        libraries = shipped_libraries(tar)
        extra = [{"name": "legacycore:deb-depends", "value": deb_field(path, "Depends")}]
        nested, deps = [], []
        if package == runtime:
            nested, deps, refs = library_components(os.path.basename(path), libraries, version, protobuf["bom-ref"])
            depends_on = refs
        else:
            if libraries:
                die(f"{os.path.basename(path)} ships {libraries}: the libraries belong in {runtime}")
            depends_on = [package_refs[0]]           # the runtime package, which it depends on
        if ships_headers(tar):
            depends_on.append("communication")       # contactci/haptic_state.h
        component = artifact_component(path, "library", package, version, purl, properties + extra, nested)
        components.append(component)
        dependencies += deps + [{"ref": component["bom-ref"], "dependsOn": depends_on}]
        package_refs.append(component["bom-ref"])

    path = tarballs[0]
    with tarfile.open(path, mode="r:gz") as tar:
        libraries = shipped_libraries(tar, strip_prefix=f"contactci-core-{version}/")
        headers = ships_headers(tar)
    nested, deps, refs = library_components(os.path.basename(path), libraries, version, protobuf["bom-ref"])
    component = artifact_component(path, "file", os.path.basename(path), version, None, properties, nested)
    components.append(component)
    dependencies += deps + [{"ref": component["bom-ref"], "dependsOn": refs + (["communication"] if headers else [])}]
    package_refs.append(component["bom-ref"])

    dependencies.append({"ref": "legacycore", "dependsOn": package_refs})

    metadata_component = {
        "type": "library",
        "bom-ref": "legacycore",
        "name": "legacycore",
        "version": version,
        "licenses": EULA,
        "externalReferences": [{"type": "vcs", "url": REPO_URL}],
    }
    revision = git_commit(ROOT)
    if revision:
        metadata_component["purl"] = f"pkg:github/Contact-Control-Interfaces/legacycore@{revision}"

    bom = {
        "bomFormat": "CycloneDX",
        "specVersion": "1.5",
        "serialNumber": f"urn:uuid:{uuid.uuid4()}",
        "version": 1,
        "metadata": {
            "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "tools": {"components": [{"type": "application", "name": "tools/write-linux-sbom.py"}]},
            "component": metadata_component,
            "properties": properties,
        },
        "components": components + [protobuf, communication_component()],
        "dependencies": dependencies,
    }

    os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)
    with open(output_file, "w", newline="\n") as f:
        json.dump(bom, f, indent=2)
        f.write("\n")
    print(f"    SBOM: {len(components)} packages for {platform} -> {output_file}")


if __name__ == "__main__":
    main(sys.argv)
