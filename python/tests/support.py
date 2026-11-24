"""
What the binding's tests share: where the repository's headers and the built
library are, and the fake daemon.

Under CTest (./build.sh --tests) the environment says where things are:

    CONTACTCI_LIBRARY            the libccic.so just built (the binding's own override)
    CONTACTCI_TEST_FAKE_DAEMON   the fake_daemon executable (test/unit/support)
    CONTACTCI_TEST_CC            the C compiler the build used

Run by hand, they fall back to build/release-linux and cc:

    cd python && python3 -m unittest discover -v
"""

import os
import shutil
import subprocess

import contactci

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BUILD_DIR = os.path.join(ROOT, "build", "release-linux")

# Where the C API's headers are in the source tree; an installed tree has them all in one include/.
INCLUDE_DIRS = [
    os.path.join(ROOT, "ccic", "include"),
    os.path.join(ROOT, "types", "include"),
    os.path.join(ROOT, "cci", "packets", "include"),
]
HAPTIC_STATE_HEADER = os.path.join(ROOT, "cci", "packets", "include", "contactci", "haptic_state.h")
C_API_HEADER = os.path.join(ROOT, "ccic", "include", "contactci.h")
STATUS_HEADER = os.path.join(ROOT, "types", "include", "ccic", "error.h")

# Core's override of the Service's socket path (cci/src/pipe_channel_posix.cpp)
SERVICE_SOCKET_VARIABLE = "CONTACTCI_SERVICE_SOCKET"


def _built(variable, name):
    path = os.environ.get(variable) or os.path.join(BUILD_DIR, name)
    if not os.path.exists(path):
        raise FileNotFoundError(f"{path} not found. Build it with ./build.sh --tests, or set {variable}")
    return path


def library_path():
    return _built(contactci.LIBRARY_ENVIRONMENT_VARIABLE, "libccic.so")


def c_compiler():
    for candidate in (os.environ.get("CONTACTCI_TEST_CC"), os.environ.get("CC"), "cc", "gcc"):
        if candidate and shutil.which(candidate):
            return candidate
    raise FileNotFoundError("no C compiler found: set CONTACTCI_TEST_CC, or install one (build-essential)")


class ScopedEnvironment:
    """Sets an environment variable until restore(), then puts back what was there."""

    def __init__(self, name, value):
        self.name = name
        self.old = os.environ.get(name)
        os.environ[name] = value   # os.environ calls putenv, so Core's getenv sees it

    def restore(self):
        if self.old is None:
            os.environ.pop(self.name, None)
        else:
            os.environ[self.name] = self.old


class FakeDaemon:
    """
    The C++ suite's fake daemon (test/unit/support/fake_daemon_posix.*), run as
    the fake_daemon program and driven over its stdin and stdout. Core is
    pointed at its socket for as long as it runs.
    """

    def __init__(self):
        self.process = subprocess.Popen(
            [_built("CONTACTCI_TEST_FAKE_DAEMON", "fake_daemon")],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True,
        )
        greeting = self.process.stdout.readline().split(" ", 1)
        if greeting[0] != "socket":
            self.process.kill()
            raise RuntimeError(f"fake_daemon did not start: {greeting!r}")
        self.socket_path = greeting[1].rstrip("\n")
        self.environment = ScopedEnvironment(SERVICE_SOCKET_VARIABLE, self.socket_path)

    def command(self, line):
        """Send one command; return what follows "ok" on the reply."""
        self.process.stdin.write(line + "\n")
        self.process.stdin.flush()
        reply = self.process.stdout.readline().rstrip("\n")
        status, _, value = reply.partition(" ")
        if status != "ok":
            raise RuntimeError(f"fake_daemon: {line!r} -> {reply!r}")
        return value

    def stop(self):
        self.environment.restore()
        self.process.stdin.close()   # end of input stops it
        self.process.stdout.close()
        self.process.wait(timeout=10)
