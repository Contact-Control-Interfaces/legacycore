"""
Core's C API (ccic/include/contactci.h) as ctypes sees it: the structs, the
status codes, the function prototypes and the loader.

Everything here mirrors a C declaration by hand, so a change to one of these
headers needs the same change here:

    cci/packets/include/contactci/haptic_state.h   HapticState, the constants
    ccic/include/contactci.h                       DeviceDescription, ClientDescription, PROTOTYPES
    types/include/ccic/error.h                     CciStatus

python/tests/test_layout.py compiles the headers and fails when they disagree.
"""

from __future__ import annotations

import ctypes
import enum
import os
import sys
import threading

# The SONAME of the C API library, which carries the major version of Core this
# binding was written against (the VERSION file). The runtime package
# libcontactci-core3 installs it; no -dev package is needed.
SONAME = "libccic.so.3"

# Set to a path, this replaces SONAME for load_library() with no argument.
LIBRARY_ENVIRONMENT_VARIABLE = "CONTACTCI_LIBRARY"

# Digit order everywhere in HapticState: field N of a group belongs to DIGITS[N].
DIGITS = ("thumb", "index", "middle", "ring", "little")

HAPTIC_STATE_SIZE = 72       # CONTACTCI_HAPTIC_STATE_SIZE
HAPTIC_REGION_SIZE = 144     # CONTACTCI_HAPTIC_REGION_SIZE: a left then a right HapticState


class HapticState(ctypes.Structure):
    """
    One hand's haptic state: struct ContactCiHapticState, 72 bytes, field for field.

    The vibration amplitudes are declared float because the header declares
    them float. The Windows Service and the C# wrapper read the same 4 bytes as
    a uint32, and which is right is undecided (INN-14); until it is, write 0.0
    there and use the vibration effect fields.

    ctypes does not range-check: a value outside 0..255 in a uint8 field wraps,
    and a float is narrowed to 32 bits.
    """

    _fields_ = [
        ("thumbForceFeedbackAmplitude", ctypes.c_float),
        ("indexForceFeedbackAmplitude", ctypes.c_float),
        ("middleForceFeedbackAmplitude", ctypes.c_float),
        ("ringForceFeedbackAmplitude", ctypes.c_float),
        ("littleForceFeedbackAmplitude", ctypes.c_float),

        ("thumbForceFeedbackPosition", ctypes.c_float),
        ("indexForceFeedbackPosition", ctypes.c_float),
        ("middleForceFeedbackPosition", ctypes.c_float),
        ("ringForceFeedbackPosition", ctypes.c_float),
        ("littleForceFeedbackPosition", ctypes.c_float),

        # float in the header; a uint32 to the Windows Service (INN-14)
        ("thumbVibrationAmplitude", ctypes.c_float),
        ("indexVibrationAmplitude", ctypes.c_float),
        ("middleVibrationAmplitude", ctypes.c_float),
        ("ringVibrationAmplitude", ctypes.c_float),
        ("littleVibrationAmplitude", ctypes.c_float),

        ("thumbVibrationEffect", ctypes.c_uint8),
        ("indexVibrationEffect", ctypes.c_uint8),
        ("middleVibrationEffect", ctypes.c_uint8),
        ("ringVibrationEffect", ctypes.c_uint8),
        ("littleVibrationEffect", ctypes.c_uint8),

        ("thumbVibrationModifier", ctypes.c_uint8),
        ("indexVibrationModifier", ctypes.c_uint8),
        ("middleVibrationModifier", ctypes.c_uint8),
        ("ringVibrationModifier", ctypes.c_uint8),
        ("littleVibrationModifier", ctypes.c_uint8),
        # 2 bytes of trailing padding, which ctypes adds as the C compiler does
    ]

    def clear(self) -> None:
        """Set every field to zero: all force and vibration off."""
        ctypes.memset(ctypes.addressof(self), 0, ctypes.sizeof(self))

    def __repr__(self) -> str:
        fields = ", ".join(f"{name}={getattr(self, name)!r}" for name, _ in self._fields_)
        return f"HapticState({fields})"


# The struct is read and written straight through a pointer into memory shared
# with the Service, so a layout that differs is refused at import, not found on
# a glove. (An explicit raise: python -O strips assert.)
if ctypes.sizeof(HapticState) != HAPTIC_STATE_SIZE:
    raise ImportError(
        f"contactci: HapticState is {ctypes.sizeof(HapticState)} bytes on this platform, not {HAPTIC_STATE_SIZE}"
    )


class DeviceDescription(ctypes.Structure):
    """struct DeviceDescription. The strings belong to Core; copy them before the next call."""

    _fields_ = [
        ("productLine", ctypes.c_char_p),
        ("serialNumber", ctypes.c_char_p),
        ("isRight", ctypes.c_bool),
        ("isConnected", ctypes.c_bool),
    ]


class ClientDescription(ctypes.Structure):
    """struct ClientDescription. The string belongs to Core; copy it before the transaction ends."""

    _fields_ = [
        ("processID", ctypes.c_uint32),
        ("processName", ctypes.c_char_p),
    ]


class CciStatus(enum.IntEnum):
    """enum CciStatus. It is positional in C: new codes are only ever appended."""

    CCI_SUCCESS = 0
    CCI_SESSION_NOT_CONNECTED = 1
    CCI_NO_DEVICE = 2
    CCI_UNKNOWN_ERROR = 3

    CCI_ERR_SESSION_INVALID_HANDLE = 4
    CCI_ERR_SESSION_NOT_CONNECTED = 5
    CCI_ERR_SESSION_ACCESS_DENIED = 6

    CCI_ERR_PIPE_FAILED_TO_OPEN = 7
    CCI_ERR_PIPE_FAILED_TO_WRITE = 8
    CCI_ERR_PIPE_FAILED_TO_READ = 9
    CCI_ERR_PIPE_FAILED_TO_FLUSH = 10

    CCI_ERR_EVENT_FAILED_TO_OPEN = 11
    CCI_ERR_EVENT_FAILED_TO_SET = 12
    CCI_ERR_EVENT_FAILED_TO_RESET = 13
    CCI_ERR_EVENT_FAILED_TO_WAIT = 14

    CCI_ERR_SHM_FAILED_TO_OPEN = 15
    CCI_ERR_SHM_FAILED_TO_MAP = 16

    CCI_ERR_SESSION_PROTOCOL_MISMATCH = 17
    CCI_ERR_PIPE_PERMISSION_DENIED = 18


_handle = ctypes.c_void_p            # CciSessionHandle and the two transaction handles
_status = ctypes.c_int               # CciStatus: a C enum is an int
_haptic_state_p = ctypes.POINTER(HapticState)

# Every function contactci.h declares: name -> (restype, argtypes).
PROTOTYPES = {
    "cci_start_device_listing_transaction": (_handle, []),
    "cci_fetch_device_listing": (_status, [_handle, _handle, ctypes.POINTER(ctypes.c_size_t)]),
    "cci_get_device_listing": (None, [_handle, ctypes.POINTER(DeviceDescription)]),
    "cci_end_device_listing_transaction": (None, [_handle]),

    "cci_get_left_device": (_status, [_handle, ctypes.POINTER(DeviceDescription)]),
    "cci_get_right_device": (_status, [_handle, ctypes.POINTER(DeviceDescription)]),

    "cci_start_client_listing_transaction": (_handle, []),
    "cci_fetch_client_listing": (_status, [_handle, _handle, ctypes.POINTER(ctypes.c_size_t)]),
    "cci_get_client_listing": (None, [_handle, ctypes.POINTER(ClientDescription)]),
    "cci_end_client_listing_transaction": (None, [_handle]),

    "cci_create_session": (_status, [ctypes.POINTER(_handle)]),
    "cci_create_readonly_haptic_session": (_status, [ctypes.POINTER(_handle)]),
    "cci_create_mutable_haptic_session": (_status, [ctypes.POINTER(_handle)]),
    "cci_close_session": (None, [_handle]),
    "cci_is_session_connected": (_status, [_handle]),

    "cci_get_session_haptic_state": (
        _status, [_handle, ctypes.POINTER(_haptic_state_p), ctypes.POINTER(_haptic_state_p)]),
    "cci_get_global_haptic_state": (
        _status, [_handle, ctypes.POINTER(_haptic_state_p), ctypes.POINTER(_haptic_state_p)]),

    "cci_signal_session_haptic_state_changed": (_status, [_handle]),
    "cci_wait_global_haptic_state_changed": (_status, [_handle, ctypes.c_int, ctypes.POINTER(ctypes.c_bool)]),

    "cci_get_session_service_version": (ctypes.c_char_p, [_handle]),
    "cci_is_session_service_interactive": (ctypes.c_bool, [_handle]),

    "cci_get_error_string": (ctypes.c_char_p, [_status]),
}

_load_lock = threading.Lock()
_library: ctypes.CDLL | None = None
_library_name: str | None = None


def load_library(path: str | os.PathLike[str] | None = None) -> ctypes.CDLL:
    """
    Load Core's C API library and declare its functions. Opening a Session does
    this by itself; call it first only to choose the library or to fail early.

    The library is `path` if given, else $CONTACTCI_LIBRARY if set and not
    empty, else the SONAME (libccic.so.3) through the dynamic loader's usual
    search. One library is loaded per process: later calls return it, and
    naming a different one raises RuntimeError.

    Raises OSError when the library can't be loaded or lacks a function.
    """
    global _library, _library_name

    name = os.fspath(path) if path is not None else os.environ.get(LIBRARY_ENVIRONMENT_VARIABLE) or None

    with _load_lock:
        if _library is not None:
            if path is not None and name != _library_name:
                raise RuntimeError(f"contactci: {_library_name} is already loaded; cannot also load {name}")
            return _library

        if name is None:
            if not sys.platform.startswith("linux"):
                raise OSError(
                    f"contactci: no default library name on {sys.platform}; pass a path to load_library() "
                    f"or set {LIBRARY_ENVIRONMENT_VARIABLE}"
                )
            name = SONAME

        try:
            # CDLL, never WinDLL: the API is cdecl everywhere but 32-bit Windows (lib_defs.h)
            library = ctypes.CDLL(name)
        except OSError as error:
            raise OSError(
                f"contactci: could not load {name} ({error}). Install Core's runtime package "
                f"(libcontactci-core3), or set {LIBRARY_ENVIRONMENT_VARIABLE} to the path of libccic.so"
            ) from error

        for function_name, (restype, argtypes) in PROTOTYPES.items():
            try:
                function = getattr(library, function_name)
            except AttributeError:
                raise OSError(
                    f"contactci: {name} does not export {function_name}; it is not Core's C API, "
                    f"or it is older than this binding"
                ) from None
            function.restype = restype
            function.argtypes = argtypes

        _library, _library_name = library, name
        return library
