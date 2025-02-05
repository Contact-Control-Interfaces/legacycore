# Contact CI Library

This document describes building this repository from source. You hopefully do not need this. Builds are handled by out GitLab CI system and are available as nuget and NPM packages, as well as the bare DLLs. Please see the Releases page on GitLab if you need the bare DLLs.

Otherwise, for minor changes you don't actually need to be able to build. You can just commit your changes and let CI handle it. Creating a release is slightly complicated, see the relevant section at the end of this file.

The build environment required is delicate and extremely un-fun. Proceed with caution, you have been warned!

# !! IMPORTANT !!

The C++ API and the C# wrapper will be published with the **same version number at the same time**. In order to maintain sanity, **do not** add APIs to the C++ library without also adding a corresponding C# wrapper. These two libraries should ideally be 100% equivalent.

## Prerequisites
This repo uses the [Communications repo](https://gitlab.contact.ci/sdk/libraries/communication) as a submodule, so remember to do a `git submodule update --init --recursive` after cloning this repo!

- This project utilizes CMake to generate its build files.
- Targets C++20 and C99 language standards.
- You will need protobuf (protoc, and libs) installed somewhere.
    - For MSVC builds, protobuf is pulled down as a vcpkg
    - For other builds (e.g. MinGW), you'll need to provide the path to protobuf as part of the CMake profile options:
        - e.g. `-DCMAKE_PREFIX_PATH="C:/Program Files (x86)/protobuf"`
- You need protobuf 3.22, which is provided in a zip file in this repo.

## Project Structure

### Protobuf
The protobuf code is generated from the `*.proto` files found in `cci/packets/`. This done in `CMakeLists.txt` using the Protobuf CMake package.

### cci
This is the source code for the C++ library.
It uses the PIMPL idiom to keep a consistent public interface and to avoid the need to leak internal types and implementation details.
The API exposed by this library is defined by the public versions of PIMPL classes found in *public_include/contactci.h*;
If you change any of the classes as part of the API (e.g. the `*Session` types) you'll need to ensure that *public_include/contactci.h*
is updated as well.

The primary means of utilizing this library is by creating an instance of `Session`, `HapticSession`, or `MutableHapticSession`

The *packets/* directory contains the git submodule for protobuf files describing the packets for talking to the service and devices.

### ccic
This is the source code fo the C library. This library depends upon and wraps `cci`.

The API functions return `CciStatus` where appropriate to indicate success or errors. This enum can be found in `types/include/ccic/error.h`

### types
This is a header-only library defining shared types used by both `cci` and `ccic`
- **types/include/ccic/lib_defs.h**
    - This defines some preprocessor macros for managing compilation using C++ vs C compilers and properly exporting or importing symbols for linkage from shared libraries (e.g. `__declspec(dllexport)` for Windows).

### test
A collection of one-off testing executables for testing various features of `cci` and `ccic`

## Building

You'll need to do some CMake configuration depending on the toolchain you're using.

### MinGW
You'll need to make sure CMake can find your protobuf installation. After following either of the below instructions, it's best to reset/clear your CMake cache and reload the CMake project.

For CLion:

- Navigate to File -> Settings -> CMake
- Ensure that the "CMake options" field includes a definition of `CMAKE_PREFIX_PATH` pointing to your protobuf installation
    - For example, `-DCMAKE_PREFIX_PATH="C:/Program Files (x86)/protobuf"`
    - Update the above path to point to your protobuf installation (the root, not the `bin/` directory)

### MSVC
(MSVC builds are no longer used, but this documentation is left until we're *sure* we don't need it anymore)

You'll need to install protobuf via vcpkg.

For CLion:

- Navigate to View -> Tool Windows -> Vcpkg
- Ensure that `protobuf` is shown under "Installed", otherwise install it by finding it under "All" (or searching for it)
- Navigate to File -> Settings -> CMake
- Ensure that the "CMake options" includes a definition of `CMAKE_TOOLCHAIN_FILE` pointing to the vcpkg build system
    - For example, `-DCMAKE_TOOLCHAIN_FILE=C:\Users\john_contactci\.clion-vcpkg\vcpkg\scripts\buildsystems\vcpkg.cmake`
    - CLion may automatically do this when you install the protobuf vcpkg
    - When reloading the CMake project, you may see warnings about `CMAKE_TOOLCHAIN_FILE` not being used. This is okay. It's only used initially when generating cmake files and is cached afterwards, and cmake defaults to generating warnings for unused flags/envvars

## Releases

If you follow the happy path, creating a release is as simple as 
```bash
git add foo.h
git commit -m "added MyFooFuncFactoryAbstractFactory for pre-blitted greebling"
git tag 2.1.15-beta
git push
git push --tags
```

Doing this creates a **publicly visible** release. 

### If you want to create a tag without a release:
`git tag 3.0.0-rc -m "<optional message> [SKIP CI]"`

If you add `[SKIP CI]` or `[CI SKIP]` to your commit message, the obvious happens. This works for regular commits as well as tags.

### Use version suffixes!
A full fat release fit for normal consumption should be just the version: `2.1.4`

If you're testing something, do `2.1.56-beta`, `2.1.58-dev`, or similar. **Do not** use feature names like `2.1.4-wav-stream`.

Your tag string is visible in the public nuget package index. If it's not a stable release, you need to mark it as such so that users know what they're getting. We also don't want to leak any information we don't have to.

Please also use a suffix like `-rc` on tags which don't get a release, or otherwise make it immediately obvious that the tag doesn't have a release. Tags *must* be a semantic version string in the above format *unless* the tag is marked `[CI SKIP]`. CI will throw an error if this is done incorrectly, and should stop an accidental release.

If you need to delete a release, you can do so from GitLab. Delete both the release from the Releases page *and* in the package repository. Contact an admin for guidance.

The happy path: use git hooks. 

[ instructions for git hooks here ]

What this does:

I strongly recommend you at least look at [the script](), but in essence it will detect when you are about to push a tag commit and quietly insert a new commit that changes CoreConductor to depend on the new version of libcore that you're about to create. It then fixes the tag reference to point at this commit before it gets pushed to the server.

Why this is done:

We want `CoreConductor 2.1.5` to depend on `libcore 2.1.5`. When you push your changes, `libcore 2.1.5` does not yet exist, and your csproj depends on `libcore 2.1.4`. We build `libcore 2.1.5` just before `CoreConductor 2.1.5`, so somewhere in the middle we have to tell CoreConductor that libcore has a new version.

Otherwise, you have to run two releases to produce a `CoreConductor 2.1.6` which depends on `libcore 2.1.5`, and a `CoreConductor 2.1.5` which is still a valid release, but will likely crash badly at runtime.

There's two ways to handle this:

CI can do this behind the scenes. It could modify the nuget package manifest to depend on the newest version. This means the actual package is now out of sync with the repo. You then have to manually update the dependency version in your local copy of the repo and push that upstream. CI could do that automatically, but then the upstream repo will become out of sync with your local copy at some indeterminate point in the next 30-60 minutes. You then need to wait for that change to happen and pull it back down before you can do any more work.

Or the developer can do this on their local copy prior to pushing to the server. You can manually edit the csproj and nuspec files, or you can let the computer do it. This way your local copy never goes out of sync with the remote repo and you don't have to futz with the csproj.

Be aware that you must commit *before* you create the tag. `git tag` will create a tag pointing at your current local `HEAD` commit. If you tag first, your release won't contain any commits which follow!