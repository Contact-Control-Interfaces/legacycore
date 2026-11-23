"""
The binding end to end against the fake daemon: Python -> libccic.so -> Unix
socket and shared memory -> the C++ suite's fake daemon
(test/unit/support/fake_daemon_posix.*), run as the fake_daemon program.

What a test writes through the binding is read back from the daemon's own
mapping of the Session's region, so a pass means the bytes reached the Service
side. No real Service or glove is involved. Linux only.
"""

import gc
import struct
import sys
import threading
import time
import unittest

import contactci

from . import support


def eventually(condition, timeout=5.0):
    """Polls until `condition` holds; Core refreshes its lists on threads of its own."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if condition():
            return True
        time.sleep(0.01)
    return condition()


@unittest.skipUnless(sys.platform.startswith("linux"), "libccic.so is built on Linux")
class LoopbackTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        contactci.load_library(support.library_path())

    def setUp(self):
        self.daemon = support.FakeDaemon()
        self.addCleanup(self.daemon.stop)
        # Whatever the test checked, Core must have kept to the contract on the wire
        self.addCleanup(lambda: self.assertEqual(self.daemon.command("errors"), ""))

    def session_state(self):
        """The left and right hand of the write Session's region, as the daemon sees them."""
        region = bytes.fromhex(self.daemon.command("session-state"))
        self.assertEqual(len(region), contactci.HAPTIC_REGION_SIZE)
        return (contactci.HapticState.from_buffer_copy(region[:72]),
                contactci.HapticState.from_buffer_copy(region[72:]))

    def set_global_state(self, left, right):
        self.daemon.command("global-state " + (bytes(left) + bytes(right)).hex())

    # -- Session ---------------------------------------------------------------

    def test_a_session_reports_the_service(self):
        self.daemon.command("version 2.3.12-test")
        with contactci.Session() as session:
            self.assertEqual(session.service_version, "2.3.12-test")
            self.assertIs(session.service_interactive, True)
            self.assertIs(session.is_connected(), True)
            self.assertIs(session.closed, False)
        self.assertEqual(self.daemon.command("requests"), "001")   # not haptic, no write access, protocol 1

    def test_left_and_right_device(self):
        self.daemon.command("devices Maestro,L-1,left,1;Maestro,R-9,right,0")
        with contactci.Session() as session:
            left = session.left_device()
            self.assertEqual(left, contactci.Device("Maestro", "L-1", False, True))
            self.assertEqual((left.product_line, left.serial_number, left.is_right, left.is_connected),
                             ("Maestro", "L-1", False, True))
            self.assertIsNone(session.right_device(), "a glove that isn't connected is no device")

            # The Service signals a change and Core refetches: the right glove connects
            self.daemon.command("devices Maestro,L-1,left,1;Maestro,R-9,right,1")
            self.assertTrue(eventually(lambda: session.right_device() is not None))
            self.assertEqual(session.right_device(), contactci.Device("Maestro", "R-9", True, True))
            self.assertEqual(left.serial_number, "L-1", "an earlier result is a copy")

    def test_device_and_client_lists(self):
        self.daemon.command("devices Maestro,L-1,left,1;Maestro,R-9,right,0")
        self.daemon.command("clients 321,Game.exe;7,glove_node")
        with contactci.Session() as session:
            self.assertEqual(session.devices(), [contactci.Device("Maestro", "L-1", False, True),
                                                 contactci.Device("Maestro", "R-9", True, False)])
            self.assertEqual(session.clients(), [contactci.Client(321, "Game.exe"), contactci.Client(7, "glove_node")])

    def test_empty_lists(self):
        with contactci.Session() as session:
            self.assertEqual(session.devices(), [])
            self.assertEqual(session.clients(), [])
            self.assertIsNone(session.left_device())

    def test_a_plain_session_has_no_haptic_state(self):
        with contactci.Session() as session:
            for name in ("left", "right", "global_left", "global_right", "signal_changed", "wait_global_changed"):
                self.assertFalse(hasattr(session, name), name)

    def test_is_connected_turns_false_when_the_service_hangs_up(self):
        with contactci.Session() as session:
            self.assertTrue(session.is_connected())
            self.daemon.command("disconnect")
            self.assertTrue(eventually(lambda: not session.is_connected()))

    # -- Opening fails -----------------------------------------------------------

    def test_access_denied_raises_its_status(self):
        self.daemon.command("grant 0")
        with self.assertRaises(contactci.CciError) as raised:
            contactci.MutableHapticSession()
        self.assertIs(raised.exception.status, contactci.CciStatus.CCI_ERR_SESSION_ACCESS_DENIED)
        self.assertIn("rejecting requested access", raised.exception.message)

    def test_another_protocol_version_raises_its_status(self):
        self.daemon.command("protocol 2")
        with self.assertRaises(contactci.CciError) as raised:
            contactci.Session()
        self.assertIs(raised.exception.status, contactci.CciStatus.CCI_ERR_SESSION_PROTOCOL_MISMATCH)

    # -- Session haptic state ----------------------------------------------------

    def test_field_writes_reach_the_service_and_a_signal_wakes_it(self):
        with contactci.MutableHapticSession() as session:
            self.assertEqual(self.daemon.command("requests"), "111")   # haptic, write access, protocol 1

            session.right.indexForceFeedbackAmplitude = 0.25
            session.left.middleVibrationEffect = 4
            session.left.middleVibrationModifier = 200

            left, right = self.session_state()
            self.assertEqual(right.indexForceFeedbackAmplitude, 0.25)
            self.assertEqual(left.middleVibrationEffect, 4)
            self.assertEqual(left.middleVibrationModifier, 200)
            self.assertEqual(left.indexForceFeedbackAmplitude, 0.0, "the left hand's field is not the right's")
            self.assertEqual(session.right.indexForceFeedbackAmplitude, 0.25, "and reads back through the view")

            self.assertEqual(self.daemon.command("session-signals"), "0", "writing alone signals nothing")
            session.signal_changed()
            self.assertEqual(self.daemon.command("session-signals"), "1")
            session.signal_changed()
            self.assertEqual(self.daemon.command("session-signals"), "2")

    def test_every_field_round_trips_at_its_own_bytes(self):
        with contactci.MutableHapticSession() as session:
            for index, (name, _) in enumerate(contactci.HapticState._fields_, start=1):
                setattr(session.right, name, index)
            _, right = self.session_state()
            for index, (name, _) in enumerate(contactci.HapticState._fields_, start=1):
                with self.subTest(field=name):
                    self.assertEqual(getattr(right, name), index)

    def test_whole_state_write_read_and_clear(self):
        command = contactci.HapticState()
        command.thumbVibrationEffect = 14
        command.littleForceFeedbackAmplitude = 0.5
        with contactci.MutableHapticSession() as session:
            session.left.write(command)
            left, right = self.session_state()
            self.assertEqual(bytes(left), bytes(command))
            self.assertEqual(bytes(right), bytes(72))

            snapshot = session.left.read()
            self.assertIsInstance(snapshot, contactci.HapticState)
            self.assertEqual(bytes(snapshot), bytes(command))
            snapshot.thumbVibrationEffect = 99          # a copy: the shared memory doesn't follow
            self.assertEqual(session.left.thumbVibrationEffect, 14)

            session.left.clear()
            self.assertEqual(bytes(self.session_state()[0]), bytes(72))

            with self.assertRaises(TypeError):
                session.left.write(b"\0" * 72)

    def test_vibration_amplitude_is_written_as_the_float_the_header_declares(self):
        # INN-14: the Windows Service reads these 4 bytes as a uint32. The binding writes what the header says.
        with contactci.MutableHapticSession() as session:
            session.left.thumbVibrationAmplitude = 0.5
            region = bytes.fromhex(self.daemon.command("session-state"))
            self.assertEqual(region[40:44], struct.pack("<f", 0.5))

    def test_a_misspelt_field_is_an_error_not_a_silent_no_op(self):
        with contactci.MutableHapticSession() as session:
            with self.assertRaises(AttributeError):
                session.left.thumbVibrationEfect = 1
            with self.assertRaises(AttributeError):
                session.left.pinkyVibrationEffect

    # -- Global haptic state -----------------------------------------------------

    def test_global_state_is_read_live_and_cannot_be_written(self):
        left, right = contactci.HapticState(), contactci.HapticState()
        left.indexForceFeedbackAmplitude = 0.25
        right.ringVibrationEffect = 3
        self.set_global_state(left, right)

        with contactci.HapticSession() as session:
            self.assertEqual(self.daemon.command("requests"), "101")   # haptic, no write access, protocol 1
            self.assertEqual(session.global_left.indexForceFeedbackAmplitude, 0.25)
            self.assertEqual(session.global_right.ringVibrationEffect, 3)
            self.assertEqual(bytes(session.global_right.read()), bytes(right))

            right.ringVibrationEffect = 8
            self.set_global_state(left, right)
            self.assertEqual(session.global_right.ringVibrationEffect, 8, "a view is the memory, not a copy")

            # The region is mapped read-only: a write through a raw pointer would kill the process
            with self.assertRaises(AttributeError):
                session.global_left.thumbVibrationEffect = 1
            with self.assertRaises(AttributeError):
                session.global_left.clear()
            with self.assertRaises(AttributeError):
                session.global_left.write(left)
            self.assertFalse(hasattr(session, "left"), "a read-only Session has no state of its own")

    def test_a_mutable_session_reads_the_global_state_too(self):
        left = contactci.HapticState()
        left.thumbVibrationEffect = 5
        self.set_global_state(left, contactci.HapticState())
        with contactci.MutableHapticSession() as session:
            self.assertEqual(session.global_left.thumbVibrationEffect, 5)
            self.assertEqual(session.left.thumbVibrationEffect, 0, "its own state is another region")

    def test_wait_global_changed(self):
        with contactci.HapticSession() as session:
            self.assertIs(session.wait_global_changed(10), False)
            self.daemon.command("global-signal")
            self.assertIs(session.wait_global_changed(10), True)
            self.assertIs(session.wait_global_changed(10), False)

    # -- Lifetime ------------------------------------------------------------------

    def test_close_twice_is_safe_and_closes_the_connection(self):
        session = contactci.MutableHapticSession()
        self.assertEqual(self.daemon.command("connections"), "1")
        session.close()
        session.close()
        self.assertIs(session.closed, True)
        self.assertTrue(eventually(lambda: self.daemon.command("connections") == "0"))
        session.__exit__(None, None, None)   # as when a `with` block ends after an explicit close()

    def test_use_after_close_raises_instead_of_crashing(self):
        session = contactci.MutableHapticSession()
        left, right, global_left = session.left, session.right, session.global_left
        left.thumbVibrationEffect = 1
        snapshot = left.read()
        session.close()

        # The regions are unmapped now: each of these would be a segmentation fault through a raw pointer
        with self.assertRaises(contactci.SessionClosedError):
            left.thumbVibrationEffect
        with self.assertRaises(contactci.SessionClosedError):
            right.thumbVibrationEffect = 1
        with self.assertRaises(contactci.SessionClosedError):
            global_left.thumbVibrationEffect
        with self.assertRaises(contactci.SessionClosedError):
            left.read()
        with self.assertRaises(contactci.SessionClosedError):
            left.write(snapshot)
        with self.assertRaises(contactci.SessionClosedError):
            left.clear()
        with self.assertRaises(contactci.SessionClosedError):
            session.left.thumbVibrationEffect
        for call in (session.signal_changed, session.is_connected, session.left_device, session.right_device,
                     session.devices, session.clients, lambda: session.wait_global_changed(0),
                     session.__enter__):
            with self.assertRaises(contactci.SessionClosedError):
                call()

        self.assertIsInstance(contactci.SessionClosedError(), contactci.ContactCiError)
        self.assertEqual(snapshot.thumbVibrationEffect, 1, "a copy taken before close stays valid")
        self.assertEqual(session.service_version, "9.9.9-test", "and so does what was read at open")
        self.assertEqual(repr(left), "HapticStateView(closed)")

    def test_a_with_block_closes_on_an_exception(self):
        with self.assertRaises(ZeroDivisionError):
            with contactci.MutableHapticSession() as session:
                1 / 0
        self.assertIs(session.closed, True)
        self.assertTrue(eventually(lambda: self.daemon.command("connections") == "0"))

    def test_a_dropped_session_is_closed_by_the_garbage_collector(self):
        session = contactci.MutableHapticSession()
        self.assertEqual(self.daemon.command("connections"), "1")
        del session
        gc.collect()
        self.assertTrue(eventually(lambda: self.daemon.command("connections") == "0"))

    def test_a_view_keeps_its_session_open(self):
        left = contactci.MutableHapticSession().left
        gc.collect()
        left.thumbVibrationEffect = 2
        self.assertEqual(self.session_state()[0].thumbVibrationEffect, 2)

    def test_close_waits_for_a_wait_in_another_thread(self):
        session = contactci.HapticSession()
        results = []
        waiter = threading.Thread(target=lambda: results.append(session.wait_global_changed(300)))
        waiter.start()
        time.sleep(0.05)       # let it get into the wait
        session.close()        # returns only once the wait has, then unmaps
        waiter.join(timeout=5)
        self.assertEqual(results, [False])
        self.assertIs(session.closed, True)

    def test_sessions_from_several_threads(self):
        with contactci.MutableHapticSession() as session:
            def work(digit):
                for value in range(200):
                    setattr(session.left, digit + "VibrationEffect", value % 256)
                    session.signal_changed()
                    session.left_device()

            threads = [threading.Thread(target=work, args=(digit,)) for digit in contactci.DIGITS]
            for thread in threads:
                thread.start()
            for thread in threads:
                thread.join(timeout=30)
            self.assertEqual(self.daemon.command("session-signals"), str(200 * len(contactci.DIGITS)))
            self.assertEqual(bytes(self.session_state()[0])[60:65], bytes([199] * 5))


if __name__ == "__main__":
    unittest.main()
