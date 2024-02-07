# Communication

Defines .proto structures for shared communications libraries.

Later there will be real code in here, but for now it's just the .proto

# Usage

## PIO

Platformio can pull any git repo as a dependency. In platformio.ini add the lines
```
lib_deps =
    git@gitlab.contact.ci:sdk/libraries/communication.git
    Nanopb

custom_nanopb_protos =
    +<.pio/libdeps/teensy40/communication/proto/haptics.proto>
    +<.pio/libdeps/teensy40/communication/proto/data.proto>

custom_nanopb_options =
    --error-on-unmatched --cpp-descriptors
```


You *must* have SSH keys correctly configured on your machine to authenticate with a private repo.

See [Gitlab docs](https://docs.gitlab.com/ee/user/ssh.html) for general guidelines, or ask software team lead for help.

# Dimension Flags

SetDimensionMessage contains a bitfield for dimension-specific flags. This allows for iterating new control protocols without touching the communication logic.

## Vibration

Flags defined in VibrationManager.h

`DIM_FLAGS_LIB_SEQ     1`
Use the built-in effect library and the sequencer. This is effectively the same as the DK3 implementation. Handler expects two bytes in the value array. Byte 0 is the effect code, byte 1 is the modifiers.

`DIM_FLAGS_RAW     2`
Message contains a raw vibration amplitude. Handler expects one byte in the value array, which is the amplitude.

## Force Feedback

Flags defined in ForceFeedbackManager.h

`DIM_FLAGS_8BIT    1`
Message contains a single amplitude byte in the values array.

`DIM_FLAGS_15BIT    2`
Alternate high-resolution mode. Values array contains two bytes which are concatenated to a uint16.