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

- **include/**
  - These are the public headers (defines the exposed API)
- **packets/**
  - The submodule containing the proto files 
- **src**
  - The source code
  - **include/**
    - The internal headers (not for external use)

### Protobuf
The protobuf code is generated from the `*.proto` files found in `packets/`. This done in `CMakeLists.txt` using the Protobuf CMake package.

### cci_lib_defs.h
This defines some preprocessor macros for managing compilation using C++ vs C compilers and properly exporting or importing symbols for linkage from shared libraries (e.g. `__declspec(dllexport)` for Windows).

### exports.cpp
This is where all the API functions are defined