# License

Copyright (c) Contact CI.

All rights reserved.

## legacycore

This repository and its contents are proprietary and confidential unless a
separate written agreement states otherwise. You may not copy, distribute,
sublicense, publish, or use this material outside authorized Contact CI work.

No warranty is provided. Use, deployment, and modification are permitted only
by authorized contributors and operators.

The packages built from this repository - `ContactCI.Maestro.libcore` and
`ContactCI.Maestro.CoreConductor` on NuGet, `@contactci/com.contactci.libcore`
and `@contactci/com.contactci.coreconductor` on npm, and the DLLs attached to
each GitHub Release - are distributed to customers under the
[Contact CI SDK EULA](https://contact.ci/pages/sdk-eula), which the npm
packages reference as `licensesUrl`. This file governs the source; the EULA
governs the binaries in a customer's hands.

## Third-party components

`libcci.dll` and `libccic.dll` are linked statically (`-static`), so the
components below are compiled *into* the shipped DLLs rather than shipped next
to them. Their terms apply to those components, not to Contact CI's own source.
The licences are the SPDX expressions MSYS2 records for each package. The exact
package versions in a given build are in that build's `sbom/bom.json`.

| Component | Where | Terms |
| --- | --- | --- |
| Protocol Buffers 3.21.12 | compiled into `libcci.dll`; built from source by `tools/build-protobuf.sh` | BSD 3-Clause |
| GCC runtime (libstdc++, libgcc), from MSYS2 `mingw-w64-x86_64-gcc` | compiled into both DLLs | `GPL-3.0-or-later WITH GCC-exception-3.1` |
| mingw-w64 winpthreads, from `mingw-w64-x86_64-winpthreads` | compiled into both DLLs (MSYS2's GCC uses the posix thread model) | `MIT AND BSD-3-Clause-Clear` |
| mingw-w64 runtime (CRT startup), from `mingw-w64-x86_64-crt` | compiled into both DLLs | `ZPL-2.1` |
| Microsoft C runtime (`msvcrt.dll`) | imported at run time, part of Windows | Windows component, not redistributed |
| communication `.proto` definitions | `cci/packets` submodule, generated into `libcci.dll` | Proprietary, Contact CI |

CoreConductor references no third-party packages; it depends only on the .NET
runtime and on libcore.

### What the third-party terms require

- **GCC runtime.** The Runtime Library Exception allows static linking into a
  proprietary DLL, provided it is built by GCC from ordinary source - which it
  is. It imposes no notice requirement on the binaries.
- **Protocol Buffers** (BSD 3-Clause) requires that binary redistributions
  reproduce its copyright notice, conditions and disclaimer "in the
  documentation and/or other materials provided with the distribution".
- **winpthreads** (MIT-style) requires its copyright and permission notice in
  all copies or substantial portions of the software.

**The packages and `build.zip` do not currently include those notices** - nor
did the packages GitLab published. Shipping a `THIRD-PARTY-NOTICES.txt` inside
each package (the protobuf `LICENSE` and the winpthreads `COPYING`) would meet
both requirements; until then, the SDK EULA or the public documentation should
carry them. Anything that changes how these components are linked - switching
to a shared protobuf, a different toolchain, another library - changes this
table: raise it rather than landing it as a packaging change.
