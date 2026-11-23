"""
Sessions with the Service, as Python objects over Core's C API.

    Session                 device and client lists, the Service's version
    HapticSession           also reads the Global haptic state
    MutableHapticSession    also writes this Session's haptic state

A Session is one connection to the Service. Close it (or use `with`) when done.
Closing only ends the connection: to stop feedback, clear the state and signal
first, rather than rely on what the Service does when a Session goes away.
"""

from __future__ import annotations

import ctypes
import threading
from typing import NamedTuple

from . import _abi
from ._abi import CciStatus, HapticState

_FIELD_NAMES = frozenset(name for name, _ in HapticState._fields_)


class ContactCiError(Exception):
    """Base class of this package's errors."""


class CciError(ContactCiError):
    """
    A call into Core returned a status other than success.

    `status` is the CciStatus (an int the binding doesn't know, if Core is
    newer than the binding) and `message` is cci_get_error_string for it.
    """

    def __init__(self, status: CciStatus | int, message: str):
        super().__init__(f"{message} ({status.name})" if isinstance(status, CciStatus) else f"{message} ({status})")
        self.status = status
        self.message = message


class SessionClosedError(ContactCiError):
    """The Session was used, or its haptic state was touched, after close()."""


class Device(NamedTuple):
    """A glove the Service knows, copied out of Core."""

    product_line: str
    serial_number: str
    is_right: bool
    is_connected: bool


class Client(NamedTuple):
    """A Session the Service is serving (the Service calls it a client), copied out of Core."""

    process_id: int
    process_name: str


def _text(value: bytes | None) -> str:
    return value.decode("utf-8", errors="replace") if value is not None else ""


def _error(library: ctypes.CDLL, code: int) -> CciError:
    try:
        status: CciStatus | int = CciStatus(code)
    except ValueError:
        status = code
    message = library.cci_get_error_string(code)
    return CciError(status, _text(message) if message is not None else "Unknown status")


class HapticStateView:
    """
    One hand's HapticState inside the Service's shared memory.

    It has the fields of HapticState, and reading or assigning one reads or
    writes the shared memory directly; nothing is copied or buffered. The
    Service acts on a Session's writes when the Session signals
    (MutableHapticSession.signal_changed).

    A view is valid only while its Session is open. After close() every
    access raises SessionClosedError: the memory is unmapped, so the view
    checks first where a raw pointer would crash the process. Views of the
    Global haptic state are read-only.
    """

    __slots__ = ("_session", "_pointer", "_writable")

    def __init__(self, session: Session, pointer: ctypes._Pointer, writable: bool):
        object.__setattr__(self, "_session", session)
        object.__setattr__(self, "_pointer", pointer)
        object.__setattr__(self, "_writable", writable)

    def _state(self, writing: bool = False) -> HapticState:
        # Called with the Session's lock held, so close() can't unmap the memory under the caller
        self._session._require_open()
        if writing and not self._writable:
            raise AttributeError("the Global haptic state is read-only; write the Session haptic state instead")
        return self._pointer.contents

    def __getattr__(self, name: str):
        if name not in _FIELD_NAMES:
            raise AttributeError(f"HapticState has no field {name!r}")
        with self._session._lock:
            return getattr(self._state(), name)

    def __setattr__(self, name: str, value) -> None:
        if name not in _FIELD_NAMES:
            raise AttributeError(f"HapticState has no field {name!r}")
        with self._session._lock:
            setattr(self._state(writing=True), name, value)

    def read(self) -> HapticState:
        """A copy of the whole state, which stays valid after the Session closes."""
        with self._session._lock:
            return HapticState.from_buffer_copy(self._state())

    def write(self, state: HapticState) -> None:
        """Replace the whole state with a copy of `state`."""
        if not isinstance(state, HapticState):
            raise TypeError(f"expected a HapticState, not {type(state).__name__}")
        with self._session._lock:
            ctypes.memmove(ctypes.addressof(self._state(writing=True)), ctypes.addressof(state), ctypes.sizeof(state))

    def clear(self) -> None:
        """Set every field to zero: all force and vibration off."""
        with self._session._lock:
            self._state(writing=True).clear()

    def __repr__(self) -> str:
        with self._session._lock:
            if self._session.closed:
                return "HapticStateView(closed)"
            return f"HapticStateView({self._state()!r})"


class Session:
    """
    A Session with the Service: its device and client lists and its version.

    Opening raises CciError when the Service can't be reached
    (CCI_ERR_PIPE_FAILED_TO_OPEN when it isn't running,
    CCI_ERR_PIPE_PERMISSION_DENIED when the user isn't in the contactci group)
    or refuses the Session, and OSError when Core's library can't be loaded.

    Methods may be called from any thread. After close() they raise
    SessionClosedError, except close() itself and `closed`.
    """

    _create = "cci_create_session"

    def __init__(self) -> None:
        # Set before anything can raise: __del__ runs even when __init__ fails
        self._handle: int | None = None
        self._lock = threading.Condition(threading.RLock())
        self._blocking_calls = 0

        self._library = _abi.load_library()
        handle = ctypes.c_void_p()
        code = getattr(self._library, self._create)(ctypes.byref(handle))
        if code != CciStatus.CCI_SUCCESS:
            raise _error(self._library, code)
        self._handle = handle.value

        try:
            # Fixed for the Session's life, so read once and still readable after close()
            self._service_version = _text(self._library.cci_get_session_service_version(self._handle))
            self._service_interactive = bool(self._library.cci_is_session_service_interactive(self._handle))
            self._open()
        except BaseException:
            self.close()
            raise

    def _open(self) -> None:
        """What a subclass fetches from its new Session."""

    def _require_open(self) -> int:
        if self._handle is None:
            raise SessionClosedError("the Session is closed")
        return self._handle

    def _check(self, code: int) -> None:
        if code != CciStatus.CCI_SUCCESS:
            raise _error(self._library, code)

    def close(self) -> None:
        """
        Close the Session and unmap its haptic state. Safe to call again.

        It waits for a wait_global_changed() in another thread to return first.
        """
        with self._lock:
            if self._handle is None:
                return
            handle, self._handle = self._handle, None   # from here every other call raises SessionClosedError
            while self._blocking_calls:
                self._lock.wait()
            self._library.cci_close_session(handle)

    @property
    def closed(self) -> bool:
        return self._handle is None

    def __enter__(self):
        self._require_open()
        return self

    def __exit__(self, *exc_info) -> None:
        self.close()

    def __del__(self) -> None:
        try:
            self.close()
        except Exception:
            pass   # interpreter shutdown: nothing useful to do with it

    def is_connected(self) -> bool:
        """False once the Service has closed this Session's connection. A Session never reconnects: open a new one."""
        with self._lock:
            code = self._library.cci_is_session_connected(self._require_open())
        if code == CciStatus.CCI_SESSION_NOT_CONNECTED:
            return False
        self._check(code)
        return True

    @property
    def service_version(self) -> str:
        """The Service's version, as it reported it when the Session opened."""
        return self._service_version

    @property
    def service_interactive(self) -> bool:
        """Whether the Service said it runs in an interactive (desktop) session."""
        return self._service_interactive

    def _side_device(self, function_name: str) -> Device | None:
        description = _abi.DeviceDescription()
        with self._lock:
            code = getattr(self._library, function_name)(self._require_open(), ctypes.byref(description))
            if code == CciStatus.CCI_NO_DEVICE:
                return None
            self._check(code)
            # Copied under the lock: the strings are Core's until the next call for this side
            return _device(description)

    def left_device(self) -> Device | None:
        """The connected left glove, or None when there isn't one."""
        return self._side_device("cci_get_left_device")

    def right_device(self) -> Device | None:
        """The connected right glove, or None when there isn't one."""
        return self._side_device("cci_get_right_device")

    def devices(self) -> list[Device]:
        """Every glove the Service knows, connected or not."""
        return self._listing("device", _abi.DeviceDescription, _device)

    def clients(self) -> list[Client]:
        """Every Session the Service is serving, this one included."""
        return self._listing("client", _abi.ClientDescription, _client)

    def _listing(self, kind: str, description_type, convert) -> list:
        library = self._library
        with self._lock:
            handle = self._require_open()
            transaction = getattr(library, f"cci_start_{kind}_listing_transaction")()
            try:
                count = ctypes.c_size_t()
                self._check(getattr(library, f"cci_fetch_{kind}_listing")(handle, transaction, ctypes.byref(count)))
                if count.value == 0:
                    return []
                descriptions = (description_type * count.value)()
                getattr(library, f"cci_get_{kind}_listing")(transaction, descriptions)
                # Copied before the transaction ends: it owns the strings
                return [convert(description) for description in descriptions]
            finally:
                getattr(library, f"cci_end_{kind}_listing_transaction")(transaction)


def _device(description: _abi.DeviceDescription) -> Device:
    return Device(
        _text(description.productLine), _text(description.serialNumber),
        bool(description.isRight), bool(description.isConnected),
    )


def _client(description: _abi.ClientDescription) -> Client:
    return Client(int(description.processID), _text(description.processName))


class HapticSession(Session):
    """A Session that also reads the Global haptic state: what the Service is applying to the gloves."""

    _create = "cci_create_readonly_haptic_session"

    def _open(self) -> None:
        super()._open()
        self._global_left, self._global_right = self._views("cci_get_global_haptic_state", writable=False)

    def _views(self, function_name: str, writable: bool) -> tuple[HapticStateView, HapticStateView]:
        left, right = ctypes.POINTER(HapticState)(), ctypes.POINTER(HapticState)()
        self._check(getattr(self._library, function_name)(self._handle, ctypes.byref(left), ctypes.byref(right)))
        return HapticStateView(self, left, writable), HapticStateView(self, right, writable)

    @property
    def global_left(self) -> HapticStateView:
        """The left hand of the Global haptic state. Read-only; valid until close()."""
        return self._global_left

    @property
    def global_right(self) -> HapticStateView:
        """The right hand of the Global haptic state. Read-only; valid until close()."""
        return self._global_right

    def wait_global_changed(self, timeout_ms: int) -> bool:
        """
        Wait until the Service signals that the Global haptic state changed.

        True if it changed since this thread's last wait (or since the Session
        opened), False after `timeout_ms` milliseconds without a change. A
        negative timeout waits forever, and a wait can't be interrupted, not
        even by Ctrl-C, so prefer short timeouts in a loop. close() from
        another thread waits for this to return.
        """
        with self._lock:
            handle = self._require_open()
            self._blocking_calls += 1
        try:
            changed = ctypes.c_bool(False)
            # Outside the lock, so other threads keep using the Session meanwhile; ctypes releases the GIL
            code = self._library.cci_wait_global_haptic_state_changed(handle, timeout_ms, ctypes.byref(changed))
        finally:
            with self._lock:
                self._blocking_calls -= 1
                self._lock.notify_all()
        self._check(code)
        return bool(changed.value)


class MutableHapticSession(HapticSession):
    """
    A Session that also writes its own haptic state, which is how an
    application commands the gloves:

        with MutableHapticSession() as session:
            session.right.indexVibrationEffect = 1
            session.signal_changed()

    For each hand the Service applies the state of whichever Session
    signalled last, the whole hand at once.
    """

    _create = "cci_create_mutable_haptic_session"

    def _open(self) -> None:
        super()._open()
        self._left, self._right = self._views("cci_get_session_haptic_state", writable=True)

    @property
    def left(self) -> HapticStateView:
        """The left hand of this Session's haptic state. Valid until close()."""
        return self._left

    @property
    def right(self) -> HapticStateView:
        """The right hand of this Session's haptic state. Valid until close()."""
        return self._right

    def signal_changed(self) -> None:
        """Tell the Service to read and apply this Session's haptic state. Writes do nothing until this."""
        with self._lock:
            self._check(self._library.cci_signal_session_haptic_state_changed(self._require_open()))
