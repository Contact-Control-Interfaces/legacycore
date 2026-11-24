"""
The binding against the libccic.so this tree built, with no Service running:
it loads with CDLL, every function the binding declares is there, the status
codes have Core's messages, and opening a Session fails with Core's status
rather than crashing. Linux only, like the library.
"""

import ctypes
import os
import subprocess
import sys
import unittest

import contactci
from contactci import _abi

from . import support


@unittest.skipUnless(sys.platform.startswith("linux"), "libccic.so is built on Linux")
class LibraryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.library = contactci.load_library(support.library_path())

    def test_it_loads_as_a_cdll(self):
        self.assertIs(type(self.library), ctypes.CDLL)
        self.assertIs(contactci.load_library(), self.library, "one library per process")

    def test_another_library_is_refused_once_one_is_loaded(self):
        with self.assertRaises(RuntimeError):
            contactci.load_library("/nonexistent/libccic.so")

    def test_every_declared_function_resolves_with_its_prototype(self):
        self.assertEqual(len(_abi.PROTOTYPES), 22)
        for name, (restype, argtypes) in _abi.PROTOTYPES.items():
            with self.subTest(function=name):
                function = getattr(self.library, name)
                self.assertEqual(function.restype, restype)
                self.assertEqual(list(function.argtypes), argtypes)

    def test_the_soname_is_this_trees_major_version(self):
        with open(os.path.join(support.ROOT, "VERSION")) as file:
            major = file.read().strip().split(".")[0]
        self.assertEqual(contactci.SONAME, f"libccic.so.{major}")

    def test_the_default_is_the_soname_through_the_loaders_search(self):
        # In a fresh interpreter, with nothing but the loader's path pointing at the build
        directory = os.path.dirname(os.path.realpath(support.library_path()))
        self.assertTrue(os.path.exists(os.path.join(directory, contactci.SONAME)))
        environment = {k: v for k, v in os.environ.items() if k != contactci.LIBRARY_ENVIRONMENT_VARIABLE}
        environment["LD_LIBRARY_PATH"] = directory
        script = "import contactci; print(contactci.load_library()._name)"
        result = subprocess.run([sys.executable, "-B", "-c", script], env=environment,
                                cwd=os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), contactci.SONAME)

    def test_a_missing_library_is_an_oserror_that_says_what_to_do(self):
        script = "import contactci\ntry:\n    contactci.load_library('/nonexistent/libccic.so')\nexcept OSError as e:\n    print(e)"
        result = subprocess.run([sys.executable, "-B", "-c", script],
                                cwd=os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("libcontactci-core3", result.stdout)
        self.assertIn(contactci.LIBRARY_ENVIRONMENT_VARIABLE, result.stdout)

    def test_every_status_has_cores_message_and_core_has_no_status_the_binding_lacks(self):
        self.assertEqual(self.library.cci_get_error_string(contactci.CciStatus.CCI_SUCCESS), b"Success")
        for status in contactci.CciStatus:
            with self.subTest(status=status.name):
                self.assertTrue(self.library.cci_get_error_string(status))
        self.assertIsNone(self.library.cci_get_error_string(max(contactci.CciStatus) + 1))


@unittest.skipUnless(sys.platform.startswith("linux"), "libccic.so is built on Linux")
class WithoutServiceTest(unittest.TestCase):
    """Core pointed at a socket nothing listens on, so a real Service on this machine isn't reached either."""

    @classmethod
    def setUpClass(cls):
        contactci.load_library(support.library_path())

    def setUp(self):
        missing = f"/tmp/contactci-no-daemon-{os.getpid()}/service.sock"
        self.environment = support.ScopedEnvironment(support.SERVICE_SOCKET_VARIABLE, missing)
        self.addCleanup(self.environment.restore)

    def test_every_kind_of_session_raises_pipe_failed_to_open(self):
        for kind in (contactci.Session, contactci.HapticSession, contactci.MutableHapticSession):
            with self.subTest(session=kind.__name__):
                with self.assertRaises(contactci.CciError) as raised:
                    kind()
                self.assertIs(raised.exception.status, contactci.CciStatus.CCI_ERR_PIPE_FAILED_TO_OPEN)
                self.assertEqual(raised.exception.message, "Failed to open pipe")
                self.assertEqual(str(raised.exception), "Failed to open pipe (CCI_ERR_PIPE_FAILED_TO_OPEN)")
                self.assertIsInstance(raised.exception, contactci.ContactCiError)


if __name__ == "__main__":
    unittest.main()
