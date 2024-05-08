# Contact CI Library

This project houses the native C and C++ libraries.

This repo uses the [Communications repo](https://gitlab.contact.ci/sdk/libraries/communication) as a submodule, so remember to do a `git submodule update --init --recursive` after cloning this repo!

## Prerequisites
- This project utilizes CMake to generate its build files.
- Targets C++20 and C99 language standards.
- You will need protobuf (protoc, and libs) installed somewhere.
  - For MSVC builds, protobuf is pulled down as a vcpkg
  - For other builds (e.g. MinGW), you'll need to provide the path to protobuf as part of the CMake profile options:
    - e.g. `-DCMAKE_PREFIX_PATH="C:/Program Files (x86)/protobuf"`

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
You'll need to install protobuf via vcpkg.

For CLion:

- Navigate to View -> Tool Windows -> Vcpkg
- Ensure that `protobuf` is shown under "Installed", otherwise install it by finding it under "All" (or searching for it)
- Navigate to File -> Settings -> CMake
- Ensure that the "CMake options" includes a definition of `CMAKE_TOOLCHAIN_FILE` pointing to the vcpkg build system
  - For example, `-DCMAKE_TOOLCHAIN_FILE=C:\Users\john_contactci\.clion-vcpkg\vcpkg\scripts\buildsystems\vcpkg.cmake`
  - CLion may automatically do this when you install the protobuf vcpkg
  - When reloading the CMake project, you may see warnings about `CMAKE_TOOLCHAIN_FILE` not being used. This is okay. It's only used initially when generating cmake files and is cached afterwards, and cmake defaults to generating warnings for unused flags/envvars