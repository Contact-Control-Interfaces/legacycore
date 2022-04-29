//
// Created by john_contactci on 3/16/2022.
//

#ifndef CONTACTCI_HAND_H
#define CONTACTCI_HAND_H

#include <cci_lib_defs.h>

typedef const void* HandTreeIndexHandle;

enum Handedness {
    Left = 0, Right = 1
};

struct HandConstants_ {
    const HandTreeIndexHandle FINGER_ROOT;

    const HandTreeIndexHandle THUMB_FINGER_METACARPAL;
    const HandTreeIndexHandle THUMB_FINGER_PROXIMAL;
    /* Thumb has no middle segment */
    const HandTreeIndexHandle THUMB_FINGER_DISTAL;

    const HandTreeIndexHandle INDEX_FINGER_METACARPAL;
    const HandTreeIndexHandle INDEX_FINGER_PROXIMAL;
    const HandTreeIndexHandle INDEX_FINGER_MIDDLE;
    const HandTreeIndexHandle INDEX_FINGER_DISTAL;

    const HandTreeIndexHandle MIDDLE_FINGER_METACARPAL;
    const HandTreeIndexHandle MIDDLE_FINGER_PROXIMAL;
    const HandTreeIndexHandle MIDDLE_FINGER_MIDDLE;
    const HandTreeIndexHandle MIDDLE_FINGER_DISTAL;

    const HandTreeIndexHandle RING_FINGER_METACARPAL;
    const HandTreeIndexHandle RING_FINGER_PROXIMAL;
    const HandTreeIndexHandle RING_FINGER_MIDDLE;
    const HandTreeIndexHandle RING_FINGER_DISTAL;

    const HandTreeIndexHandle LITTLE_FINGER_METACARPAL;
    const HandTreeIndexHandle LITTLE_FINGER_PROXIMAL;
    const HandTreeIndexHandle LITTLE_FINGER_MIDDLE;
    const HandTreeIndexHandle LITTLE_FINGER_DISTAL;
};

CCI_API extern const struct HandConstants_ HandConstants;

// Marshaling global variables in C# is basically a manual process using
// GetProdAddress, a symbol lookup, and manual byte-wise copying/marshaling.
// We can avoid all that manual work if we expose the global var through a function
// TODO should we return a pointer rather than the entire struct?
CCI_API_FUNC(struct HandConstants_) cci_hand_constants_get();

#endif //CONTACTCI_HAND_H
