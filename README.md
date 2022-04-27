# Contact CI Library

This project houses the native C and C++ libraries.

There are 4 cmake submodules:
 - **comms**
   - A library that handles the communication protocols and implementations for how the binary representation of a haptic Atom is encoded and transmitted to the actuating device (e.g. our Maestro glove). 
 - **lib**
   - A header-only library that provides templated implementations of the haptic `Atom`s, `Frame`s, `Effect`s, `Player` to play `Effect`s, and `EffectBuilder` to build `Effect`s.
   - Depends on `comms`
 - **lib-impl**
   - A concrete implementation of the currently available haptic `Atom` types and the requisite implementation details to be able to play those haptic `Atom`s. (Current implements vibration and force feedback).
 - **clib-impl**
   - A C linkage wrapper around `lib-impl` for linkage compatibilty without C++-style name mangling. This will primarily serve as the interface to other languages/runtimes such as C# and Java's JNI for Android builds (e.g. Meta Quest 2)

At the root of the project you'll find a cmake project that includes the other 4 notes above and a single `main.cpp` file that includes some rudamentary testing logic.