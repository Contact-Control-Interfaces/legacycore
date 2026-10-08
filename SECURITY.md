# Security

## Reporting a vulnerability

**Do not open a public issue or pull request for a security problem, and do
not describe it in a commit message.**

Report it privately on this repository:

> Security -> Report a vulnerability

That opens a draft security advisory visible only to you and the repository's
maintainers. (It needs private vulnerability reporting enabled under Settings ->
Code security. If the button is missing, ask a maintainer directly and keep the
details out of public channels.)

Please include:

- the affected version: the package version, or for bare DLLs the release
  whose `SHA256SUMS.txt` matches them
- how the library is consumed: C++ (`libcci`), C (`libccic`), or C#
  (CoreConductor), and whether the host process runs elevated
- what an attacker gains: code execution or a crash *in the host application*,
  impersonation, information disclosure, haptics control
- the smallest reproduction you have: the sequence of calls, or the bytes a
  hostile pipe server or shared-memory writer sends
- the Windows service version it was talking to, if any

You will get an acknowledgement and an assessment on the advisory. Please keep
the details there until a fixed version ships.

## Scope

In scope: `libcci` and `libccic` (`cci/`, `ccic/`, `types/`), the CoreConductor
wrapper, the packages built from them, and the build and release tooling
(`install.ps1`, `build.ps1`, `tools/`, `.github/workflows/ci.yml`).

The Windows service and the protocol definitions have their own repositories.
A bug on the service's side of the pipe belongs to
[windows-service](https://github.com/Contact-Control-Interfaces/windows-service);
a protocol design issue to
[communication](https://github.com/Contact-Control-Interfaces/communication).

## Trust boundary

These libraries are loaded **into other people's processes** - games, Unity and
Unreal applications, research tools - which may run as any user, including
elevated. A memory-safety bug here is a bug in every application that ships the
SDK, and it runs with that application's privileges.

They talk to the Windows service over two channels, and neither authenticates
the other end:

1. **The named pipe `\\.\pipe\contact-ci-service`** (`cci/src/pipe_channel.cpp`).
   The library opens it by name, so it connects to whichever process created
   that name - the service, or anything that created it first, for instance
   while the service is stopped. Nothing checks the server's identity.
2. **Shared memory and named events**, opened by names that the *pipe server
   sends back* in its session response (`cci/src/session.cpp`,
   `shared_memory.cpp`, `named_event.cpp`). The service widens their ACLs so
   unprivileged clients can attach, which means other local processes can read
   and write them too.

So everything arriving from the pipe or the shared memory is untrusted input,
from a peer that may not be the service. As the code stands:

- **Message lengths are taken from the wire.** `Channel::receive_delimited`
  reads a 32-bit length from the packet header and `PipeChannel::receive`
  allocates a buffer of that size before reading. A hostile server can make the
  host application allocate up to 4 GB, or exhaust memory, per request.
- **Parse failures are ignored.** `ParseFromString`'s result is not checked, so
  a malformed or truncated message becomes a default-valued response rather
  than an error.
- **The pipe is opened without a security quality of service.**
  `NamedPipe::NamedPipe` passes no `SECURITY_SQOS_PRESENT` flags to
  `CreateFile`, so the server may impersonate the client at the default
  `SecurityImpersonation` level. Opening with
  `SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION` would cap that.
- **Shared-memory contents are not validated.** `HapticStateManager` hands
  out `HapticState` references that point straight into a mapping any process
  with access can write, so the values can also change while the caller is
  reading them. Anything that indexes, sizes or branches on them has to copy
  them first and treat the copy as hostile.

Denial of service is a real finding: a local process that can crash or hang
the library takes down the host application, not just haptics.

## Supply chain and build integrity

The `sbom` job validates it against the schema and checks every hash against
the files the release publishes. 
- **Secret scanning.** gitleaks scans the full history on every pipeline and
  publishes a redacted `gitleaks-report.json`. It does not block. If a secret
  was ever committed, rotating it comes first - removing the commit does not
  un-leak it.
- **Publishing credentials.**
  - npm uses trusted publishing (OIDC): there is no npm token to steal, and
    each package version carries provenance linking it to the workflow run that
    built it.
  - nuget.org uses the `NUGET_PUBLISH_KEY` secret. Scope that key to the two
    `ContactCI.Maestro.*` packages, push only, with an expiry.
  - GitHub Packages uses the job's `GITHUB_TOKEN`.
  - The submodule is read with a GitHub App installation token scoped to this
    repository and `communication`; the `SUBMODULE_TOKEN` PAT is a fallback.
    Prefer the App - a PAT carries its owner's access everywhere.
  - Every job requests the narrowest `permissions:` it needs, and checkout does
    not persist credentials.

## Handling a confirmed vulnerability

1. Fix on a `bugfix/*` branch. Keep the pull request description and commit
   messages free of exploit detail until the fix has shipped.
2. Ship it as a release ([RELEASE.md](RELEASE.md)) - a patch version, so the
   CoreConductor dependency ranges pick it up automatically.
3. Publish the security advisory with the fixed version. If the service or the
   protocol also needed changing, say which versions go together.
4. If a credential was exposed, rotate it before the fix merges.
