//
// Created by john_contactci on 3/20/2024.
//

#pragma once

// HapticState is one hand's haptic state as it sits in the Service's shared
// memory. Its layout is defined once, for Core and the Linux Service alike, by
// communication's include/contactci/haptic_state.h (the cci/packets submodule):
// struct ContactCiHapticState, 72 bytes, the same fields, names and types
// HapticState has always had, and byte-identical to the Windows Service's view
// (DEV-16). Shipping this header means shipping contactci/haptic_state.h too.
#include <contactci/haptic_state.h>

typedef struct ContactCiHapticState HapticState;
