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

This package includes `ContactCI.Maestro.corelib` as a dependency. This is hosted in the same repo, and should automatically download as part of the `nuget restore` operation.

The `corelib` package contains the C++ DLLs as a reusable component. This is intended for non-C# applications to consume.

# Usage

First, call `MaestroSessionManager.StartSession`, which will return `true` if the Windows service is alive and offers the client a session.

From there, the functions are mostly self-explanatory.

Internally, the current haptic state is stored in a section of memory shared between the service and client. We use named pipes to transfer data and signal events. These concepts are exposed in a sane C# way with `ref` parameters and native `event`s. You shouldn't have to know or care about the internal unsafe code.

To subscribe to haptic state change events, you must call `StartHapticWatcher`. This spawns a Task on the thread pool that waits on the named event and dispatches events. To cancel this thread, you must call `StopHapticWatcher`.

To read the current haptics, call `GetHaptics`. You'll get a haptic state struct for each hand, or null if that hand is not present.

The `SetHaptics` call is similar, though you must explicitly tell the service which hand to apply haptics to.