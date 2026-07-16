#!/usr/bin/env python3
"""
Cross-check the simulator's telemetry encoder against the firmware's.

The simulator re-implements the 9-bit word in Python so it can run without a
toolchain. Two implementations of one format is exactly how a format quietly
drifts, so these are the same vectors asserted in
firmware/test/test_core/test_core.cpp. If the C++ encoder ever changes and this
file is not updated, this test fails.

Usage:
    python3 tools/test_simulator.py

Author: Nisa Maasoglu
License: MIT
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from simulator import classify_path, encode_telemetry  # noqa: E402

UNKNOWN, RESPONSIVE, SILENT = 0, 1, 2

# (alert, survivor, distance_cm, ack) -> expected word
VECTORS = [
    ((False, UNKNOWN, 100, False), "000011001"),
    ((True, RESPONSIVE, 10, True), "010101111"),
    ((True, SILENT, 0, False), "011000001"),
]

PATHS = [
    (0, "00"),      # no echo is its own state, not "clear"
    (-3, "00"),
    (14, "01"),     # blocked
    (15, "11"),     # exactly at the boundary
    (200, "11"),
]

failures = 0


def check(condition, name):
    global failures
    if condition:
        print(f"  ok   {name}")
    else:
        failures += 1
        print(f"  FAIL {name}")


def main() -> int:
    print("\nsimulator encoder vs firmware vectors\n=====================================")

    for args, expected in VECTORS:
        got = encode_telemetry(*args)
        check(got == expected, f"{args} -> {expected} (got {got})")

    for distance, expected in PATHS:
        got = classify_path(distance)
        check(got == expected, f"path {distance} cm -> {expected} (got {got})")

    for args, _ in VECTORS:
        word = encode_telemetry(*args)
        check(len(word) == 9, f"{args} produces exactly 9 bits")
        check(word[0] == "0" and word[8] == "1", f"{args} carries both markers")

    print(f"=====================================\n{failures} failures\n")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
