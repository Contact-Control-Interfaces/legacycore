## Maestro Core Conductor

This is a thin P/Invoke wrapper around the C++ legacycore DLLs. With this you can develop C# applications against the Maestro Windows service.

# Installation

You need to add our private nuget repository to your project. You can do this through your IDE in various ways, through the command line, or through a global config file. The simplest approach is to add a `nuget.config` file to the root of your project with the following:

```xml
<?xml version="1.0" encoding="utf-8"?>
<configuration>
  <packageSources>
    <add key="ContactCI" value="https://gitlab.contact.ci/api/v4/projects/52/packages/nuget/index.json" />
  </packageSources>
</configuration>
```

(you can also just copy the `nuget.config` from this project)

Then you can add the `ContactCI.Maestro.CoreConductor` nuget package to your project.

This package includes `ContactCI.Maestro.libcore` as a dependency. This is hosted in the same repo, and should automatically download as part of the `nuget restore` operation.

The `libcore` package contains the C++ DLLs as a reusable component. This is intended for non-C# applications to consume.

These libraries are also published as NPM packages.

# Usage

First, call `MaestroSessionManager.StartSession`, which will return `true` if the Windows service is alive and offers the client a session. There is a corresponding `StopSession` to de-register your client with the service. Please ensure your client application can gracefully handle all exit conditions and closes its session properly.

From there, the functions are mostly self-explanatory.

Internally, the current haptic state is stored in a section of memory shared between the service and client. An event system notifies the clients and host when a state change has happened or is requested, respectively. The state object which is exposed by this API is a *copy* of that state, so you'll need to rely on the events to make sure you're current.

To subscribe to haptic state change events, you must call `StartHapticWatcher`. This spawns a Task on the thread pool that waits on the named event and dispatches events. To cancel this thread, you must call `StopHapticWatcher`. `StopSession` will internally stop the haptic watcher if it is active.

To read the current haptics, call `GetHaptics`. You'll get a haptic state struct for each hand, or null if that hand is not present.

The `SetHaptics` call is similar, though you must explicitly tell the service which hand to apply haptics to. This *immediately* signals the service to apply haptics to the selected glove, if it's online.

# Developing
See [BUILDING.md](../BUILDING.md) (only available internally)