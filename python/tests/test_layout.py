"""
The binding's ctypes mirrors against the C headers themselves (INN-281).

HapticState is read and written straight through a pointer into memory shared
with the Service, so the Python struct has to agree with the C one byte for
byte. Nothing here repeats the header's numbers and trusts them: a C program
generated from the Python declarations is compiled against the real headers and
prints what the compiler makes of them (every sizeof and offsetof, the
CONTACTCI_* constants, every CciStatus value), and the headers are also read as
text, so a field, status or function added in C and not in Python fails too.

Needs a C compiler and the cci/packets submodule; no library, Service or glove.
"""

import ctypes
import os
import re
import subprocess
import tempfile
import unittest

import contactci
from contactci import _abi

from . import support

STRUCTS = {
    "HapticState": contactci.HapticState,
    "DeviceDescription": contactci.DeviceDescription,
    "ClientDescription": contactci.ClientDescription,
}

MACROS = [
    "CONTACTCI_DIGIT_COUNT",
    "CONTACTCI_HAPTIC_STATE_SIZE",
    "CONTACTCI_HAPTIC_STATE_FORCE_FEEDBACK_AMPLITUDE_OFFSET",
    "CONTACTCI_HAPTIC_STATE_FORCE_FEEDBACK_POSITION_OFFSET",
    "CONTACTCI_HAPTIC_STATE_VIBRATION_AMPLITUDE_OFFSET",
    "CONTACTCI_HAPTIC_STATE_VIBRATION_EFFECT_OFFSET",
    "CONTACTCI_HAPTIC_STATE_VIBRATION_MODIFIER_OFFSET",
    "CONTACTCI_HAPTIC_REGION_SIZE",
    "CONTACTCI_HAPTIC_REGION_LEFT_OFFSET",
    "CONTACTCI_HAPTIC_REGION_RIGHT_OFFSET",
]

# The field groups of HapticState, each with the header's constant for its thumb field
GROUPS = {
    "ForceFeedbackAmplitude": "CONTACTCI_HAPTIC_STATE_FORCE_FEEDBACK_AMPLITUDE_OFFSET",
    "ForceFeedbackPosition": "CONTACTCI_HAPTIC_STATE_FORCE_FEEDBACK_POSITION_OFFSET",
    "VibrationAmplitude": "CONTACTCI_HAPTIC_STATE_VIBRATION_AMPLITUDE_OFFSET",
    "VibrationEffect": "CONTACTCI_HAPTIC_STATE_VIBRATION_EFFECT_OFFSET",
    "VibrationModifier": "CONTACTCI_HAPTIC_STATE_VIBRATION_MODIFIER_OFFSET",
}

# The vibration amplitudes are float or uint32 depending on INN-14; the binding follows the header
C_TYPES = {"float": ctypes.c_float, "uint8_t": ctypes.c_uint8, "uint32_t": ctypes.c_uint32}


def probe_source():
    """A C program that prints the compiler's view of everything the binding mirrors."""
    lines = ["#include <stddef.h>", "#include <stdio.h>", "#include <contactci.h>", "", "int main(void) {"]
    for c_name, structure in STRUCTS.items():
        lines.append(f'    printf("sizeof {c_name} %zu\\n", sizeof({c_name}));')
        for field, _ in structure._fields_:
            lines.append(f'    printf("offsetof {c_name}.{field} %zu\\n", offsetof({c_name}, {field}));')
            lines.append(f'    printf("sizeof {c_name}.{field} %zu\\n", sizeof((({c_name} *) 0)->{field}));')
    for macro in MACROS:
        lines.append(f'    printf("macro {macro} %ld\\n", (long) {macro});')
    for status in contactci.CciStatus:
        lines.append(f'    printf("status {status.name} %d\\n", (int) {status.name});')
    lines += ["    return 0;", "}", ""]
    return "\n".join(lines)


def compile_and_run_probe():
    """{"sizeof HapticState": 72, "offsetof HapticState.thumbVibrationEffect": 60, "macro ...": ..., ...}"""
    with tempfile.TemporaryDirectory(prefix="contactci-layout-") as directory:
        source = os.path.join(directory, "probe.c")
        program = os.path.join(directory, "probe")
        with open(source, "w") as file:
            file.write(probe_source())
        command = [support.c_compiler(), "-std=c11", "-Wall", "-Wno-unused"]
        command += [f"-I{include}" for include in support.INCLUDE_DIRS]
        command += [source, "-o", program]
        compiled = subprocess.run(command, capture_output=True, text=True)
        if compiled.returncode != 0:
            raise AssertionError(
                "the layout probe did not compile, so the binding names something the headers don't have:\n"
                + compiled.stderr
            )
        output = subprocess.run([program], capture_output=True, text=True, check=True).stdout
    facts = {}
    for line in output.splitlines():
        key, _, value = line.rpartition(" ")
        facts[key] = int(value)
    return facts


def without_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", "", text)


def read_header(path):
    with open(path) as file:
        return without_comments(file.read())


class LayoutTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.c = compile_and_run_probe()

    def test_haptic_state_is_72_bytes(self):
        self.assertEqual(ctypes.sizeof(contactci.HapticState), 72)
        self.assertEqual(self.c["sizeof HapticState"], 72)
        self.assertEqual(self.c["macro CONTACTCI_HAPTIC_STATE_SIZE"], 72)
        self.assertEqual(contactci.HAPTIC_STATE_SIZE, 72)

    def test_every_field_is_where_the_c_compiler_puts_it(self):
        for c_name, structure in STRUCTS.items():
            self.assertEqual(ctypes.sizeof(structure), self.c[f"sizeof {c_name}"], c_name)
            for field, _ in structure._fields_:
                with self.subTest(struct=c_name, field=field):
                    descriptor = getattr(structure, field)
                    self.assertEqual(descriptor.offset, self.c[f"offsetof {c_name}.{field}"])
                    self.assertEqual(descriptor.size, self.c[f"sizeof {c_name}.{field}"])

    def test_haptic_state_has_the_headers_fields_in_order_with_their_types(self):
        header = read_header(support.HAPTIC_STATE_HEADER)
        body = re.search(r"struct ContactCiHapticState\s*\{(.*?)\}", header, flags=re.DOTALL).group(1)
        declared = re.findall(r"(\w+)\s+(\w+)\s*;", body)
        self.assertEqual(len(declared), body.count(";"), "a declaration in the struct this test can't read")
        self.assertEqual(len(declared), 25)

        self.assertEqual([name for _, name in declared], [name for name, _ in contactci.HapticState._fields_])
        for (c_type, name), (_, python_type) in zip(declared, contactci.HapticState._fields_):
            with self.subTest(field=name):
                self.assertIs(python_type, C_TYPES[c_type])

    def test_groups_are_at_the_headers_offsets_one_field_per_digit(self):
        self.assertEqual(len(contactci.DIGITS), self.c["macro CONTACTCI_DIGIT_COUNT"])
        for group, macro in GROUPS.items():
            for index, digit in enumerate(contactci.DIGITS):
                with self.subTest(group=group, digit=digit):
                    descriptor = getattr(contactci.HapticState, digit + group)
                    self.assertEqual(descriptor.offset, self.c[f"macro {macro}"] + index * descriptor.size)
        self.assertEqual(len(GROUPS) * len(contactci.DIGITS), len(contactci.HapticState._fields_))

    def test_a_region_is_a_left_then_a_right_hand(self):
        self.assertEqual(contactci.HAPTIC_REGION_SIZE, self.c["macro CONTACTCI_HAPTIC_REGION_SIZE"])
        self.assertEqual(contactci.HAPTIC_REGION_SIZE, 2 * ctypes.sizeof(contactci.HapticState))
        self.assertEqual(self.c["macro CONTACTCI_HAPTIC_REGION_LEFT_OFFSET"], 0)
        self.assertEqual(self.c["macro CONTACTCI_HAPTIC_REGION_RIGHT_OFFSET"], ctypes.sizeof(contactci.HapticState))

    def test_status_codes_are_the_headers(self):
        for status in contactci.CciStatus:
            with self.subTest(status=status.name):
                self.assertEqual(int(status), self.c[f"status {status.name}"])

        header = read_header(support.STATUS_HEADER)
        body = re.search(r"typedef enum CciStatus\s*\{(.*?)\}", header, flags=re.DOTALL).group(1)
        declared = [entry.split("=")[0].strip() for entry in body.split(",") if entry.strip()]
        self.assertEqual(declared, [status.name for status in contactci.CciStatus])

    def test_every_function_the_header_declares_has_a_prototype(self):
        header = read_header(support.C_API_HEADER)
        declared = re.findall(r"CCI_API_FUNC\([^)]*\)\s*(\w+)\s*\(", header)
        self.assertEqual(len(declared), 22)
        self.assertEqual(sorted(declared), sorted(_abi.PROTOTYPES))


class HapticStateTest(unittest.TestCase):
    """HapticState on its own: it needs no library."""

    def test_a_new_state_is_all_off(self):
        self.assertEqual(bytes(contactci.HapticState()), bytes(72))

    def test_fields_land_at_their_bytes(self):
        state = contactci.HapticState()
        state.indexForceFeedbackAmplitude = 1.0
        state.littleVibrationModifier = 7
        raw = bytes(state)
        self.assertEqual(raw[4:8], bytes.fromhex("0000803f"))   # 1.0f, little-endian
        self.assertEqual(raw[69], 7)
        self.assertEqual(raw[70:72], bytes(2))                   # the trailing padding

    def test_clear_turns_everything_off(self):
        state = contactci.HapticState()
        state.thumbForceFeedbackAmplitude = 0.5
        state.ringVibrationEffect = 12
        state.clear()
        self.assertEqual(bytes(state), bytes(72))


if __name__ == "__main__":
    unittest.main()
