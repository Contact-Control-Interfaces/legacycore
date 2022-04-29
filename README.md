# Contact CI Library

This project houses the native C and C++ libraries.

## Prerequisites
- This project utilizes CMake to generate its build files.
- Targets C++20 and C99 language standards.

## Project Structure

The subprojects are organized into three groups: 
- **interfaces**
- **libraries**
- **tests**

## Interfaces
Interfaces are intended to be header-only libraries. They produce no binaries and simply define the API interface between libraries or some other component.

The CMake projects names for interfaces follow the convention of being prefixed with `interface_`. For example, the `cci` interface has the CMake project name of `interface_cci`.

### cci
The `cci` interface is a C++ header-only library that provides templated implementations of the haptic `Atom`s, `Frame`s, `Effect`s, `Player` to play `Effect`s, and `EffectBuilder` to build `Effect`s.

*Dependencies:*
- *comms*
  - *Needed to "play" an `Effect`. This dependency is likely to change as the details of the `comms` interface and `Effect` encodings are further developed.*

### comms
The `comms` interface is a C++ header-only library that defines the `Communicator` abstract class. `Communicator` serves as the interface for transmission of a binary-encoded `Effect` to the actuating device (e.g. our Maestro glove).

This interface provides no implementations of `Communicator`. The details of how `Effect`s are encoded into a sequence of bytes (`uint8_t`s) is not within the scope of this interface.
For details on `Effect` encoding, see the template specializations of `AtomPlayer::play` provided in the `cci` library (not the interface).

### library_core
This `library_core` interface is a single header, `cci_lib_defs.h`, that defines some preprocessor macros for managing compilation using C++ vs C compilers and properly exporting or importing symbols for linkage from shared libraries (e.g. `__declspec(dllexport)` for Windows).

## Libraries
The libraries provide concrete implementations of some interfaces and produce binaries in the form of shared libraries. The choice between shared vs static libraries has not been fully decided and may change.

### cci
The `cci` library provides a concrete implementation of the `cci` interface by providing two `Atom` implementations: `VibrationAtom` and `ForceFeedbackAtom`. It also provides the appropriate template specializations that accompany the implemented `Atom`s.

*Dependencies:*
- *library_core*
  - *Preprocessor macros are used to control exporting/importing of library symbols in headers during compilation.*
- *cci*
  - *This library largely serves as an implementation of this interface.*

### ccic
The `ccic` library provides a C wrapper around the `cci` library. This allows linkage without C++-style name mangling.

The C++ class member functions exposed in `cci` are wrapped in global functions. These functions are prefixed with `cci_` to avoid name clashing, and follow the pattern of `cci_[type]_[action]`. For example, `cci_effect_builder_create` for creating an instance of `EffectBuilder` and obtaining a referencing `EffectBuilderHandle`, as explained further in the next paragraph.

C++ types from `cci` are represented as opaque pointer types, which are obtained by calling the corresponding "create" function for the type. These "create" functions dynamically allocate memory on the heap, so these objects must be explicitly deallocated by calling the corresponding "destroy" function. 
For example, a `VibrationAtom` from `cci` is represented by a `VibrationAtomHandle`. A `VibrationAtomHandle` can be obtained by creating a new vibration atom using `cci_atom_vibration_create`, and must later be released by calling `cci_atom_vibration_destroy`.

This will primarily be used as the bridge for other technical stacks and languages, such as C# for Unity or Kotlin for Android for Meta Quest 2.

*Dependencies:*
- *library_core*
  - *Preprocessor macros are used to control exporting/importing of library symbols in headers during compilation.*
- *cci*
  - *This library wraps the C++ `cci` library*
- *comms_stdout*
  - *This serves as the implementation of the *comms* interface required by *cci* in order to play `Effect`s. See `comms` interface above; this dependency will likely change.

### comms_stdout
The `comms_stdout` library is an implementation of the `comms` interface. It only outputs the encoded `Effects` as the individual `Atom` byte representations to `stdout`. These bytes are arranged in columns with each column representing an `Atom` type.
For now, the first column is `ForceFeedbackAtom`s amd the second is `VibrationAtom`s.

This is likely to change along with the rest of communications and encoding of `Effect`s and `Atom`s.

*Dependencies:*
- *library_core*
  - *Preprocessor macros are used to control exporting/importing of library symbols in headers during compilation.*
- *comms*
  - *This library implements the `comms` interface.*

## Tests
The tests are simple executables that utilize the libraries above to serve as simple tests of their functionality.

Ideally, these tests will be more thoroughly expanded to include a solid coverage of library functionality via unit tests.

### cci
Serves as the test executable utilizing the `cci` library.

### ccic
Serves as the test executable utilizing the `ccic` library. This project utilizes a C compiler as opposed to the C++ compiler used by the majority of the project.