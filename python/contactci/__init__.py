"""
contactci: Python binding over Core's C API (libccic), for talking to the
Contact CI Service that owns the haptic gloves.

Pure ctypes and standard library only, so it needs no compiler and runs
unchanged on Python 3.10 and later. Importing it loads nothing: Core's library
is loaded when the first Session opens (or by load_library), so HapticState
can be used, and tested against, on a machine without Core.

    import contactci

    with contactci.MutableHapticSession() as session:
        session.right.indexVibrationEffect = 1
        session.signal_changed()
"""

from ._abi import (
    DIGITS,
    HAPTIC_REGION_SIZE,
    HAPTIC_STATE_SIZE,
    LIBRARY_ENVIRONMENT_VARIABLE,
    SONAME,
    CciStatus,
    ClientDescription,
    DeviceDescription,
    HapticState,
    load_library,
)
from ._session import (
    CciError,
    Client,
    ContactCiError,
    Device,
    HapticSession,
    HapticStateView,
    MutableHapticSession,
    Session,
    SessionClosedError,
)

__all__ = [
    "DIGITS",
    "HAPTIC_REGION_SIZE",
    "HAPTIC_STATE_SIZE",
    "LIBRARY_ENVIRONMENT_VARIABLE",
    "SONAME",
    "CciError",
    "CciStatus",
    "Client",
    "ClientDescription",
    "ContactCiError",
    "Device",
    "DeviceDescription",
    "HapticSession",
    "HapticState",
    "HapticStateView",
    "MutableHapticSession",
    "Session",
    "SessionClosedError",
    "load_library",
]
