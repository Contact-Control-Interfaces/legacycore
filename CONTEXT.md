# Contact CI Core

The client side of talking to Contact CI haptic gloves: a library that applications link to, which hands their haptic commands to a background Service that owns the gloves.

## Language

**Core**:
The client library that applications link to in order to talk to the Service. `cci` (C++) and `ccic` (flat C) are its build targets, not separate concepts.
_Avoid_: legacycore, libcore

**Service**:
The long-running background process that owns the connected gloves and serves Core's sessions. It has two implementations: the Windows Service and `linux-service` on Linux.
_Avoid_: Maestro, Maestro service, daemon

**Client**:
An application that uses Core to talk to the Service. One Client may hold several Sessions at once.

**Session**:
One connection from a Client to the Service, through which all of that Client's interaction happens. What the Service and Core's client list call a "client" is really a Session.
_Avoid_: Client (when meaning a single connection)

### Haptic state

**Global haptic state**:
The haptic state the Service is applying to the gloves, readable by every Session.
_Avoid_: read region

**Session haptic state**:
One Session's requested haptic state, which the Service merges with every other Session's to produce the Global haptic state. It has the same shape as the Global haptic state.
_Avoid_: write region
