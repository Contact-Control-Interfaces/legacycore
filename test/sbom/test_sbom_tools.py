"""
Tests for the SBOM tools the sbom CI job relies on (INN-162):

  tools/write-linux-sbom.py   a fragment per build-linux leg
  tools/merge-sbom.py         Windows SBOM + Linux fragments -> one bom.json
  tools/check-sbom.py         the release gate: every file, every hash

Each test builds its own small fixture packages: .debs (an ar archive with
gzip tars, which dpkg-deb reads), tarballs and a Windows SBOM. Where dpkg-deb
is installed (CI's Ubuntu runners) write-linux-sbom.py uses it as it does in
build-linux; elsewhere the .debs are read in Python instead.

    python3 -m unittest discover -s test/sbom -v
"""

import contextlib
import hashlib
import importlib.util
import io
import json
import os
import shutil
import tarfile
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
VERSION = "3.0.4-beta.7"
DEB_VERSION = "3.0.4~beta.7"
LEGACYCORE_COMMIT = "1" * 40
PACKETS_COMMIT = "2" * 40
DEPENDS = "libc6 (>= 2.34), libgcc-s1 (>= 3.0), libstdc++6 (>= 12)"


def load(name):
    spec = importlib.util.spec_from_file_location(name.replace("-", "_"), os.path.join(ROOT, "tools", f"{name}.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def sha256(data):
    return hashlib.sha256(data).hexdigest()


# ---------------------------------------------------------------------------
# Fixture packages
# ---------------------------------------------------------------------------

def tar_gz(files, prefix="./"):
    buffer = io.BytesIO()
    with tarfile.open(fileobj=buffer, mode="w:gz") as tar:
        for path, data in files.items():
            info = tarfile.TarInfo(prefix + path)
            info.size, info.mode = len(data), 0o644
            tar.addfile(info, io.BytesIO(data))
    return buffer.getvalue()


def ar(members):
    out = b"!<arch>\n"
    for name, data in members:
        out += f"{name:<16}{0:<12}{0:<6}{0:<6}{'100644':<8}{len(data):<10}`\n".encode() + data
        if len(data) % 2:
            out += b"\n"
    return out


def write_deb(path, package, arch, files, deb_version=DEB_VERSION):
    control = (f"Package: {package}\nVersion: {deb_version}\nArchitecture: {arch}\n"
               f"Maintainer: Test <test@example.com>\nDepends: {DEPENDS}\nDescription: fixture\n").encode()
    with open(path, "wb") as f:
        f.write(ar([("debian-binary", b"2.0\n"),
                    ("control.tar.gz", tar_gz({"control": control})),
                    ("data.tar.gz", tar_gz(files))]))


def libraries(tag):
    """Distinct library contents per package, as stripped and unstripped copies differ."""
    return {"libcci.so.3.0.4": f"libcci {tag}".encode(), "libccic.so.3.0.4": f"libccic {tag}".encode()}


def write_leg(directory, distro="ubuntu22.04", arch="amd64", version=VERSION):
    """The three files build.sh --package writes for one Ubuntu release and architecture."""
    os.makedirs(directory, exist_ok=True)
    tag = f"{distro}_{arch}"
    runtime = {f"usr/lib/{name}": data for name, data in libraries(f"stripped {tag}").items()}
    dev = {"usr/include/contactci.h": b"/* C API */", "usr/include/contactci/haptic_state.h": b"/* layout */"}
    tree = {f"lib/{name}": data for name, data in libraries(f"unstripped {tag}").items()}
    tree.update({"include/contactci.h": b"/* C API */"})
    names = {
        "runtime": f"libcontactci-core3_{version}_{distro}_{arch}.deb",
        "dev": f"libcontactci-core-dev_{version}_{distro}_{arch}.deb",
        "tarball": f"contactci-core_{version}_{distro}_{arch}.tar.gz",
    }
    deb_version = version.replace("-", "~", 1)
    write_deb(os.path.join(directory, names["runtime"]), "libcontactci-core3", arch, runtime, deb_version)
    write_deb(os.path.join(directory, names["dev"]), "libcontactci-core-dev", arch, dev, deb_version)
    with open(os.path.join(directory, names["tarball"]), "wb") as f:
        f.write(tar_gz(tree, prefix=f"contactci-core-{version}/"))
    return names


def windows_bom(directory, protobuf, version=VERSION):
    """A Windows SBOM as write-sbom.ps1 writes it, for one build.zip in directory."""
    os.makedirs(directory, exist_ok=True)
    with open(os.path.join(directory, "build.zip"), "wb") as f:
        f.write(b"windows build")
    with open(os.path.join(directory, "SHA256SUMS.txt"), "w") as f:
        f.write("not described: written from the same files\n")
    return {
        "bomFormat": "CycloneDX", "specVersion": "1.5", "version": 1,
        "metadata": {
            "tools": {"components": [{"type": "application", "name": "tools/write-sbom.ps1"}]},
            "component": {"type": "library", "bom-ref": "legacycore", "name": "legacycore", "version": version,
                          "purl": f"pkg:github/Contact-Control-Interfaces/legacycore@{LEGACYCORE_COMMIT}"},
        },
        "components": [
            {"type": "file", "bom-ref": "file:build.zip", "name": "build.zip", "version": version,
             "hashes": [{"alg": "SHA-256", "content": sha256(b"windows build")}],
             "properties": [{"name": "legacycore:artifact", "value": "build.zip"}]},
            {"type": "file", "bom-ref": "file:libcci.dll", "name": "libcci.dll", "version": version},
            protobuf,
            {"type": "data", "bom-ref": "communication", "name": "communication", "version": PACKETS_COMMIT,
             "purl": f"pkg:github/Contact-Control-Interfaces/communication@{PACKETS_COMMIT}"},
        ],
        "dependencies": [
            {"ref": "file:libcci.dll", "dependsOn": [protobuf["bom-ref"], "communication"]},
            {"ref": "file:build.zip", "dependsOn": ["file:libcci.dll"]},
            {"ref": "legacycore", "dependsOn": ["file:build.zip"]},
        ],
    }


# ---------------------------------------------------------------------------
# Reading the fixture .debs without dpkg-deb
# ---------------------------------------------------------------------------

def ar_members(path):
    with open(path, "rb") as f:
        data = f.read()
    members, i = {}, 8
    while i < len(data):
        header = data[i:i + 60]
        name, size = header[:16].decode().strip().rstrip("/"), int(header[48:58])
        members[name] = data[i + 60:i + 60 + size]
        i += 60 + size + (size & 1)
    return members


def python_deb_tar(path):
    return tarfile.open(fileobj=io.BytesIO(ar_members(path)["data.tar.gz"]), mode="r:gz")


def python_deb_field(path, field):
    with tarfile.open(fileobj=io.BytesIO(ar_members(path)["control.tar.gz"]), mode="r:gz") as tar:
        control = tar.extractfile("./control").read().decode()
    for line in control.splitlines():
        key, _, value = line.partition(":")
        if key == field:
            return value.strip()
    return ""


# ---------------------------------------------------------------------------

class SbomTestCase(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.tmp)
        self.saved_version = os.environ.get("CCI_VERSION")
        os.environ["CCI_VERSION"] = VERSION
        self.addCleanup(self.restore_version)

        self.write = load("write-linux-sbom")
        self.write.git_commit = lambda directory: LEGACYCORE_COMMIT
        self.write.submodule_commit = lambda path: PACKETS_COMMIT
        if shutil.which("dpkg-deb") is None:
            self.write.deb_tar = python_deb_tar
            self.write.deb_field = python_deb_field
        self.merge = load("merge-sbom")
        self.check = load("check-sbom")

    def restore_version(self):
        if self.saved_version is None:
            os.environ.pop("CCI_VERSION", None)
        else:
            os.environ["CCI_VERSION"] = self.saved_version

    def path(self, *parts):
        return os.path.join(self.tmp, *parts)

    def run_tool(self, module, argv):
        """(exit code, stderr) of module.main(argv)."""
        stderr = io.StringIO()
        with contextlib.redirect_stderr(stderr), contextlib.redirect_stdout(io.StringIO()):
            try:
                module.main(argv)
                code = 0
            except SystemExit as exit:
                code = exit.code
        return code, stderr.getvalue()

    def fragment(self, leg, **kwargs):
        """Writes a leg's packages and its fragment; returns (packages dir, fragment path, file names)."""
        directory = self.path("linux", leg)
        names = write_leg(directory, **kwargs)
        output = self.path("fragments", f"{leg}.json")
        code, err = self.run_tool(self.write, ["write-linux-sbom.py", directory, output])
        self.assertEqual(code, 0, err)
        return directory, output, names

    def windows(self):
        bom = windows_bom(self.path("windows"), self.write.protobuf_component())
        path = self.path("windows-bom.json")
        with open(path, "w") as f:
            json.dump(bom, f)
        return path

    def load_json(self, path):
        with open(path) as f:
            return json.load(f)

    def gate(self, bom, directories):
        """check-sbom.py over the files of every directory together, as the sbom job sees them."""
        artifacts = self.path("artifacts")
        shutil.rmtree(artifacts, ignore_errors=True)
        os.makedirs(artifacts)
        for directory in directories:
            for name in os.listdir(directory):
                shutil.copy(os.path.join(directory, name), artifacts)
        out = io.StringIO()
        return self.check.check(bom, artifacts, out=out), out.getvalue()


class WriteLinuxSbomTest(SbomTestCase):
    def test_describes_each_package_and_the_libraries_it_ships(self):
        directory, output, names = self.fragment("ubuntu-22.04")
        bom = self.load_json(output)
        packages = {c["properties"][0]["value"]: c for c in bom["components"] if c.get("properties")}
        self.assertEqual(set(packages), set(names.values()))
        for name, component in packages.items():
            with open(os.path.join(directory, name), "rb") as f:
                self.assertEqual(component["hashes"][0]["content"], sha256(f.read()), name)

        runtime = packages[names["runtime"]]
        self.assertEqual(
            {c["name"]: c["hashes"][0]["content"] for c in runtime["components"]},
            {f"usr/lib/{n}": sha256(d) for n, d in libraries("stripped ubuntu22.04_amd64").items()})
        self.assertIn({"name": "legacycore:deb-depends", "value": DEPENDS}, runtime["properties"])
        self.assertTrue(runtime["purl"].startswith(f"pkg:deb/contactci/libcontactci-core3@{DEB_VERSION}?arch=amd64"))

        tarball = packages[names["tarball"]]
        self.assertEqual(
            {c["name"]: c["hashes"][0]["content"] for c in tarball["components"]},
            {f"lib/{n}": sha256(d) for n, d in libraries("unstripped ubuntu22.04_amd64").items()})
        self.assertNotIn("components", packages[names["dev"]])

        refs = {c["bom-ref"] for c in bom["components"]}
        self.assertIn("communication", refs)
        self.assertIn(self.write.protobuf_component()["bom-ref"], refs)
        legacycore = next(d for d in bom["dependencies"] if d["ref"] == "legacycore")
        self.assertEqual(len(legacycore["dependsOn"]), 3)

    def assertRefuses(self, directory, message):
        code, err = self.run_tool(self.write, ["write-linux-sbom.py", directory, self.path("out.json")])
        self.assertEqual(code, 1)
        self.assertIn(message, err)
        self.assertFalse(os.path.exists(self.path("out.json")))

    def test_refuses_a_file_it_does_not_describe(self):
        directory = self.path("leg")
        write_leg(directory)
        open(os.path.join(directory, "notes.txt"), "w").close()
        self.assertRefuses(directory, "notes.txt")

    def test_refuses_packages_of_another_version(self):
        directory = self.path("leg")
        write_leg(directory, version="3.0.4-beta.6")
        self.assertRefuses(directory, f"expected {VERSION}")

    def test_refuses_a_missing_tarball(self):
        directory = self.path("leg")
        names = write_leg(directory)
        os.remove(os.path.join(directory, names["tarball"]))
        self.assertRefuses(directory, "expected a runtime .deb, a dev .deb and a tarball")

    def test_refuses_packages_of_two_platforms(self):
        directory = self.path("leg")
        names = write_leg(directory)
        os.remove(os.path.join(directory, names["tarball"]))
        write_leg(self.path("other"), arch="arm64")
        shutil.copy(self.path("other", f"contactci-core_{VERSION}_ubuntu22.04_arm64.tar.gz"), directory)
        self.assertRefuses(directory, "mixes platforms")

    def test_refuses_libraries_in_the_dev_package(self):
        directory = self.path("leg")
        names = write_leg(directory)
        write_deb(os.path.join(directory, names["dev"]), "libcontactci-core-dev", "amd64",
                  {"usr/lib/libcci.so.3.0.4": b"misplaced"})
        self.assertRefuses(directory, "the libraries belong in libcontactci-core3")


class MergeSbomTest(SbomTestCase):
    def test_merges_every_fragment_and_lists_shared_components_once(self):
        _, first, first_names = self.fragment("ubuntu-22.04")
        _, second, second_names = self.fragment("ubuntu-24.04-arm", distro="ubuntu24.04", arch="arm64")
        merged = self.path("sbom", "bom.json")
        code, err = self.run_tool(self.merge, ["merge-sbom.py", merged, self.windows(), first, second])
        self.assertEqual(code, 0, err)

        bom = self.load_json(merged)
        refs = [c["bom-ref"] for c in bom["components"]]
        self.assertEqual(refs.count("communication"), 1)
        self.assertEqual(refs.count(self.write.protobuf_component()["bom-ref"]), 1)
        artifacts = {p["value"] for c in bom["components"] for p in c.get("properties", [])
                     if p["name"] == "legacycore:artifact"}
        self.assertEqual(artifacts, {"build.zip"} | set(first_names.values()) | set(second_names.values()))
        legacycore = [d for d in bom["dependencies"] if d["ref"] == "legacycore"]
        self.assertEqual(len(legacycore), 1)
        self.assertEqual(len(legacycore[0]["dependsOn"]), 7)

    def edited_fragment(self, edit):
        _, path, _ = self.fragment("ubuntu-22.04")
        bom = self.load_json(path)
        edit(bom)
        with open(path, "w") as f:
            json.dump(bom, f)
        return path

    def assertRefuses(self, fragment, message):
        merged = self.path("merged.json")
        code, err = self.run_tool(self.merge, ["merge-sbom.py", merged, self.windows(), fragment])
        self.assertEqual(code, 1)
        self.assertIn(message, err)
        self.assertFalse(os.path.exists(merged))

    def test_refuses_fragments_built_from_another_communication_commit(self):
        def edit(bom):
            component = next(c for c in bom["components"] if c["bom-ref"] == "communication")
            component["version"] = "3" * 40
        self.assertRefuses(self.edited_fragment(edit), "disagree on communication")

    def test_refuses_a_fragment_of_another_legacycore_version(self):
        def edit(bom):
            bom["metadata"]["component"]["version"] = "3.0.4-beta.6"
        self.assertRefuses(self.edited_fragment(edit), "describes legacycore version")

    def test_refuses_a_fragment_of_another_legacycore_commit(self):
        def edit(bom):
            bom["metadata"]["component"]["purl"] = "pkg:github/Contact-Control-Interfaces/legacycore@" + "4" * 40
        self.assertRefuses(self.edited_fragment(edit), "describes legacycore purl")

    def test_refuses_a_bom_ref_used_twice(self):
        def edit(bom):
            bom["components"][0]["components"][0]["bom-ref"] = "file:build.zip"
        self.assertRefuses(self.edited_fragment(edit), "more than one component")

    def test_refuses_a_dependency_on_nothing(self):
        def edit(bom):
            bom["dependencies"].append({"ref": "legacycore", "dependsOn": ["file:gone.deb"]})
        self.assertRefuses(self.edited_fragment(edit), "file:gone.deb")


class CheckSbomTest(SbomTestCase):
    def merged(self, legs):
        fragments, directories = [], [self.path("windows")]
        windows = self.windows()
        for leg, kwargs in legs:
            directory, fragment, _ = self.fragment(leg, **kwargs)
            fragments.append(fragment)
            directories.append(directory)
        merged = self.path("sbom", "bom.json")
        code, err = self.run_tool(self.merge, ["merge-sbom.py", merged, windows, *fragments])
        self.assertEqual(code, 0, err)
        return merged, directories

    LEGS = [("ubuntu-22.04", {}), ("ubuntu-24.04-arm", {"distro": "ubuntu24.04", "arch": "arm64"})]

    def test_passes_when_every_file_is_described_with_its_hash(self):
        merged, directories = self.merged(self.LEGS)
        ok, out = self.gate(merged, directories)
        self.assertTrue(ok, out)
        self.assertEqual(out.count("ok        "), 7)
        self.assertNotIn("SHA256SUMS.txt", out)

    def test_fails_when_a_file_changed_after_the_sbom_was_written(self):
        merged, directories = self.merged(self.LEGS)
        changed = os.path.join(directories[1], f"libcontactci-core-dev_{VERSION}_ubuntu22.04_amd64.deb")
        with open(changed, "ab") as f:
            f.write(b"x")
        ok, out = self.gate(merged, directories)
        self.assertFalse(ok)
        self.assertIn(f"MISMATCH  libcontactci-core-dev_{VERSION}_ubuntu22.04_amd64.deb", out)

    def test_fails_when_a_legs_fragment_is_left_out(self):
        merged, directories = self.merged(self.LEGS[:1])
        write_leg(self.path("linux", "ubuntu-24.04-arm"), distro="ubuntu24.04", arch="arm64")
        ok, out = self.gate(merged, directories + [self.path("linux", "ubuntu-24.04-arm")])
        self.assertFalse(ok)
        self.assertEqual(out.count("MISSING   "), 3)
        self.assertIn(f"MISSING   contactci-core_{VERSION}_ubuntu24.04_arm64.tar.gz", out)

    def test_fails_when_a_described_file_is_not_there(self):
        merged, directories = self.merged(self.LEGS)
        os.remove(os.path.join(directories[2], f"contactci-core_{VERSION}_ubuntu24.04_arm64.tar.gz"))
        ok, out = self.gate(merged, directories)
        self.assertFalse(ok)
        self.assertIn(f"MISMATCH  contactci-core_{VERSION}_ubuntu24.04_arm64.tar.gz", out)
        self.assertIn("file missing", out)

    def test_fails_when_a_file_is_described_twice(self):
        merged, directories = self.merged(self.LEGS)
        bom = self.load_json(merged)
        bom["components"].append(dict(bom["components"][0], **{"bom-ref": "file:again"}))
        with open(merged, "w") as f:
            json.dump(bom, f)
        ok, out = self.gate(merged, directories)
        self.assertFalse(ok)
        self.assertIn("DUPLICATE build.zip", out)


if __name__ == "__main__":
    unittest.main()
