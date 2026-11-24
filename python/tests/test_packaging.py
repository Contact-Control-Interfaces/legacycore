"""
The binding's packaging agrees with the rest of the tree: it carries Core's
version, and the python3-contactci .deb (debian/control) asks for the Python
and the Core library the binding is written for. Reads files only: it needs
no library, compiler or dpkg.

What the built .deb holds and whether it installs and loads the library is
checked on a clean Ubuntu by tools/check-linux-packages.sh.
"""

import os
import re
import unittest

import contactci

from . import support

PYTHON_PACKAGE = "python3-contactci"


def read(*parts):
    with open(os.path.join(support.ROOT, *parts), encoding="utf-8") as file:
        return file.read()


def project_table():
    """The [project] table of python/pyproject.toml as {key: text of a string value}.
    (Not tomllib: that is Python 3.11, and Ubuntu 22.04 has 3.10.)"""
    table, inside = {}, False
    for line in read("python", "pyproject.toml").splitlines():
        if line.startswith("["):
            inside = line.strip() == "[project]"
        elif inside:
            match = re.match(r'^([A-Za-z-]+)\s*=\s*"([^"]*)"', line)
            if match:
                table[match.group(1)] = match.group(2)
    return table


def control_paragraph(package):
    """The fields of one binary package in debian/control, continuation lines joined."""
    for paragraph in re.split(r"\n\s*\n", read("debian", "control")):
        fields, key = {}, None
        for line in paragraph.splitlines():
            if line.startswith("#"):
                continue
            if line[:1] in (" ", "\t") and key:
                fields[key] += " " + line.strip()
            elif ":" in line:
                key, _, value = line.partition(":")
                fields[key] = value.strip()
        if fields.get("Package") == package:
            return fields
    raise AssertionError(f"debian/control has no {package} package")


class PackagingTest(unittest.TestCase):
    def setUp(self):
        self.version = read("VERSION").strip()
        self.depends = [d.strip() for d in control_paragraph(PYTHON_PACKAGE)["Depends"].split(",")]

    def test_pyproject_carries_cores_version(self):
        self.assertEqual(project_table()["version"], self.version,
                         "python/pyproject.toml's version and the VERSION file change together")

    def test_the_deb_depends_on_exactly_its_own_build_of_the_library(self):
        # The same major version as the SONAME the binding loads, and the same build:
        # the ctypes mirrors in _abi.py are checked against that build's headers.
        major = self.version.split(".")[0]
        self.assertEqual(contactci.SONAME, f"libccic.so.{major}")
        self.assertIn(f"libcontactci-core{major} (= ${{binary:Version}})", self.depends)
        self.assertEqual(control_paragraph(f"libcontactci-core{major}")["Architecture"],
                         control_paragraph(PYTHON_PACKAGE)["Architecture"],
                         "one .deb per Ubuntu release and architecture, like the library it depends on")

    def test_the_deb_and_pyproject_ask_for_the_same_python(self):
        requires = project_table()["requires-python"]
        self.assertRegex(requires, r"^>=\d+\.\d+$")
        self.assertIn(f"python3 (>= {requires[2:]})", self.depends)


if __name__ == "__main__":
    unittest.main()
