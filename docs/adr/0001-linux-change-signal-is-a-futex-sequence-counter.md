# Linux change signal is a futex sequence counter, and never drops a change

On Windows, every change signal (Session haptic state, Global haptic state, client and device lists) is a manual-reset named event fired as `Set()` then `Reset()`. A signal fired while the waiter is busy is lost. Because the Service waits on each Session's event with no timeout, losing the last write of a burst leaves the glove on stale state until the Client writes again. On Linux we deliberately do **not** copy this. Each signal is a 32-bit sequence counter at offset 0 of a small named shm object, whose name travels in the existing event-name fields of session init. The signaller increments the counter and calls `FUTEX_WAKE` for all waiters. A waiter remembers the last value it saw and returns immediately if the counter has already moved, so no waiter ever misses a change. Readers already re-read the whole state on every wake, so a guaranteed wake costs nothing.

## Considered Options

- **Process-shared condvar + mutex in shm**: rejected. It needs robust-mutex recovery when a Client dies holding the lock, and it still needs a counter to avoid missed wakes.
- **An `eventfd` per waiter passed with `SCM_RIGHTS`**: rejected. It changes the session-init handshake, and the Service would have to track one fd per waiter for the broadcast signals.
- **A "changed" message over the control socket**: rejected. It's a protocol change and only covers the Core → Service direction, so a second mechanism would still be needed.

## Consequences

- Futex is Linux-only, not POSIX, which is acceptable because Linux is the only non-Windows target.
- The counter layout ("32-bit counter at offset 0") is part of the Core–Service contract and lives in the `communication` submodule next to the socket path.
- The 144-byte haptic state regions stay byte-identical to Windows.

## Update, 23 Oct 2026

`communication` 1.1.0 (DEV-106, implemented in Core by DEV-147) passes a write Session's own region and counter as sealed memfds with `SCM_RIGHTS` on the session-init reply. So the handshake does pass descriptors after all, and the Session haptic state counter is a passed memfd, not a named object. That doesn't change this decision: each signal is still a futex sequence counter in a 4-byte object, named or passed. The other reason against an `eventfd` per waiter still holds. The broadcast signals (Global haptic state, Device List, Client List) are still named and global, so the Service would have to track one `eventfd` per waiter for them.
