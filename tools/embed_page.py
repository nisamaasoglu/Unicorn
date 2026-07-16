#!/usr/bin/env python3
"""
Embed the command-centre page into the firmware.

panel/index.html is the single source of truth for the UI: the simulator serves
that file directly, and this script bakes the very same bytes into a PROGMEM
string that the ESP32 serves. Keeping one copy means the page a reviewer opens
against the simulator is the page the rover actually serves.

Usage:
    python3 tools/embed_page.py

Regenerates firmware/include/web_page.h. Run it after editing panel/index.html.

Author: Nisa Maasoglu
License: MIT
"""

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SOURCE = ROOT / "panel" / "index.html"
TARGET = ROOT / "firmware" / "include" / "web_page.h"

HEADER = """/*
 * GENERATED FILE - DO NOT EDIT BY HAND.
 *
 * Produced by tools/embed_page.py from panel/index.html.
 * Edit the panel, then re-run the script.
 *
 * License: MIT
 */

#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

const char COMMAND_CENTRE_HTML[] PROGMEM = R"rawliteral(
"""

FOOTER = """)rawliteral";

#endif  // WEB_PAGE_H
"""


def main() -> int:
    if not SOURCE.exists():
        print(f"error: {SOURCE} not found", file=sys.stderr)
        return 1

    html = SOURCE.read_text(encoding="utf-8")

    # A raw string literal ends at the delimiter; if the page ever contains it,
    # the generated header would break in a confusing way. Fail loudly instead.
    if ')rawliteral"' in html:
        print("error: the page contains the raw-literal delimiter", file=sys.stderr)
        return 1

    TARGET.parent.mkdir(parents=True, exist_ok=True)
    TARGET.write_text(HEADER + html + FOOTER, encoding="utf-8")

    print(f"embedded {len(html):,} bytes -> {TARGET.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
