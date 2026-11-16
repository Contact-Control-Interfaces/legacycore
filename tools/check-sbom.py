#!/usr/bin/env python3
"""
check-sbom.py
---------------------------------------------------------------
Checks that an SBOM describes exactly the files in a directory: the gate the
sbom CI job runs over `packages` and every `linux-packages-*` before release.

    tools/check-sbom.py <bom.json> <artifacts dir>

Each top-level component with a legacycore:artifact property names a file;
that file must exist with the component's SHA-256 (MISMATCH otherwise). Every
file in the directory, other than SHA256SUMS.txt, must be named by one
(MISSING otherwise). Exits 1 if anything is wrong, after listing all of it.
"""

import hashlib
import json
import os
import sys

IGNORED = {"SHA256SUMS.txt"}       # written from the same files, after the SBOM


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def described(bom):
    """(file, SHA-256) for every component that names a released file."""
    for component in bom.get("components", []):
        names = [p["value"] for p in component.get("properties", []) if p.get("name") == "legacycore:artifact"]
        if names:
            hashes = [h["content"] for h in component.get("hashes", []) if h.get("alg") == "SHA-256"]
            yield names[0], hashes[0] if hashes else None


def check(bom_path, artifacts_dir, out=sys.stdout):
    with open(bom_path, encoding="utf-8-sig") as f:
        bom = json.load(f)
    entries = list(described(bom))
    ok = True
    seen = set()
    for name, expected in entries:
        if name in seen:
            print(f"DUPLICATE {name} is described more than once", file=out)
            ok = False
        seen.add(name)
        path = os.path.join(artifacts_dir, name)
        actual = sha256_file(path) if os.path.isfile(path) else None
        if actual is not None and actual == expected:
            print(f"ok        {name}", file=out)
        else:
            print(f"MISMATCH  {name} (SBOM {expected}, file {actual or 'missing'})", file=out)
            ok = False
    for name in sorted(os.listdir(artifacts_dir)):
        if name not in IGNORED and name not in seen:
            print(f"MISSING   {name} is not in the SBOM", file=out)
            ok = False
    return ok


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("usage: check-sbom.py <bom.json> <artifacts dir>", file=sys.stderr)
        sys.exit(2)
    sys.exit(0 if check(sys.argv[1], sys.argv[2]) else 1)
