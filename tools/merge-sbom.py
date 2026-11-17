#!/usr/bin/env python3
"""
merge-sbom.py
---------------------------------------------------------------
Merges the Windows SBOM (tools/write-sbom.ps1) and the Linux fragments
(tools/write-linux-sbom.py, one per build-linux leg) into the one
CycloneDX 1.5 bom.json that the sbom CI job validates, checks every
released file against, and uploads as the `sbom` artifact.

    tools/merge-sbom.py <output> <windows bom.json> <linux fragment>...

Components that more than one input describes (protobuf, the communication
commit) appear once. They must agree on name, version and purl: two legs
built from different sources would be a broken release, so that fails here,
as does any input describing a different legacycore version or commit.
"""

import json
import os
import sys
import uuid
from datetime import datetime, timezone


def die(message):
    print(f"merge-sbom.py: {message}", file=sys.stderr)
    sys.exit(1)


def identity(component):
    return {key: component.get(key) for key in ("name", "version", "purl")}


def all_refs(components):
    for component in components:
        yield component["bom-ref"]
        yield from all_refs(component.get("components", []))


def main(argv):
    if len(argv) < 4:
        die("usage: merge-sbom.py <output> <windows bom.json> <linux fragment>...")
    output, paths = argv[1], argv[2:]
    boms = []
    for path in paths:
        with open(path, encoding="utf-8-sig") as f:      # PowerShell 5.1 may write a BOM
            boms.append((path, json.load(f)))

    (base_path, base), fragments = boms[0], boms[1:]
    product = base["metadata"]["component"]
    for path, bom in fragments:
        theirs = bom["metadata"]["component"]
        for key in ("name", "version", "purl"):
            if theirs.get(key) != product.get(key):
                die(f"{path} describes legacycore {key} {theirs.get(key)!r}, {base_path} {product.get(key)!r}")

    components = list(base["components"])
    by_ref = {c["bom-ref"]: c for c in components}
    for path, bom in fragments:
        for component in bom["components"]:
            ref = component["bom-ref"]
            if ref not in by_ref:
                by_ref[ref] = component
                components.append(component)
            elif identity(by_ref[ref]) != identity(component):
                die(f"{path} and an earlier input disagree on {ref}: {identity(component)} vs {identity(by_ref[ref])}")

    refs = list(all_refs(components))
    duplicates = sorted({r for r in refs if refs.count(r) > 1})
    if duplicates:
        die(f"bom-refs used by more than one component: {duplicates}")

    # One entry per ref, its dependsOn the union of every input's, in order.
    dependencies = {}
    for _, bom in boms:
        for entry in bom.get("dependencies", []):
            merged = dependencies.setdefault(entry["ref"], [])
            merged += [d for d in entry.get("dependsOn", []) if d not in merged]
    known = set(refs) | {"legacycore"}
    dangling = sorted({d for ref, deps in dependencies.items() for d in [ref] + deps if d not in known})
    if dangling:
        die(f"dependencies name refs no component has: {dangling}")

    tools = []
    for _, bom in boms:
        for tool in bom["metadata"].get("tools", {}).get("components", []):
            if tool not in tools:
                tools.append(tool)
    tools.append({"type": "application", "name": "tools/merge-sbom.py"})

    merged = {
        "bomFormat": "CycloneDX",
        "specVersion": "1.5",
        "serialNumber": f"urn:uuid:{uuid.uuid4()}",
        "version": 1,
        "metadata": {
            "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "tools": {"components": tools},
            "component": product,
        },
        "components": components,
        "dependencies": [{"ref": ref, "dependsOn": deps} for ref, deps in dependencies.items()],
    }
    os.makedirs(os.path.dirname(os.path.abspath(output)), exist_ok=True)
    with open(output, "w", encoding="utf-8", newline="\n") as f:
        json.dump(merged, f, indent=2)
        f.write("\n")
    print(f"    SBOM: {len(components)} components from {len(boms)} inputs -> {output}")


if __name__ == "__main__":
    main(sys.argv)
