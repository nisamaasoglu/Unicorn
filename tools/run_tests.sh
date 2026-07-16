#!/usr/bin/env bash
# Build and run the unicorn_core tests with nothing but a host compiler.
#
# The mission logic has no Arduino dependency on purpose, so it needs no
# toolchain, no board and no PlatformIO install to verify.
#
# Usage:  ./tools/run_tests.sh
#
# License: MIT

set -euo pipefail
cd "$(dirname "$0")/.."

echo "== firmware compile check =="
# Type-checks src/main.cpp against the API stubs in tools/stubs. Catches syntax
# errors, typos, wrong argument counts and type mismatches without the ESP32
# toolchain. Not a substitute for `pio run -e fm-devkit`.
g++ -std=c++11 -fsyntax-only -Wall -Wextra \
    -I tools/stubs -I firmware/include -I firmware/lib/unicorn_core \
    firmware/src/main.cpp
echo "main.cpp type-checks clean"

echo "== unicorn_core (C++) =="
g++ -std=c++11 -Wall -Wextra \
    -I firmware/lib/unicorn_core \
    firmware/lib/unicorn_core/unicorn_core.cpp \
    firmware/test/test_core/test_core.cpp \
    -o /tmp/unicorn_core_tests
/tmp/unicorn_core_tests

echo "== simulator encoder (Python) =="
python3 tools/test_simulator.py
