#!/usr/bin/env python3
"""
Unicorn rover simulator.

Serves the same page and the same HTTP interface as the ESP32, backed by a model
of the rover instead of the rover. It exists so the command centre can be opened
and driven without the hardware - the robot is a one-off built from a solar
panel, an L298N and four gearmotors, and nobody reviewing this repository has one.

Every response carries "source": "simulator", and the panel prints
SIMULATOR - NO ROVER CONNECTED in amber whenever it sees that. Nothing here is
ever presented as a live reading.

Usage:
    python3 tools/simulator.py                 # survivor answers after 6 s
    python3 tools/simulator.py --voice-delay 0 # survivor answers immediately
    python3 tools/simulator.py --voice-delay 99 # nobody answers -> SILENT path
    python3 tools/simulator.py --port 8080

Author: Nisa Maasoglu
License: MIT
"""

import argparse
import json
import math
import pathlib
import random
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

ROOT = pathlib.Path(__file__).resolve().parent.parent
PANEL = ROOT / "panel" / "index.html"

# Mirrors lib/unicorn_core/unicorn_core.h. tools/test_simulator.py checks this
# implementation against the same vectors the C++ test suite uses.
OBSTACLE_CM = 15
LISTEN_WINDOW_MS = 15000
MIC_THRESHOLD = 900

# Mirrors firmware/include/config.h
SPEED_CALIBRATED = False          # mirrors SPEED_CALIBRATED in config.h
DRIVE_SPEED_CM_S = 48.0
TURN_RATE_DEG_S = 120.0
TURN_DRIFT_CM_S = 3.0
DRIFT_FRACTION = 0.15 if SPEED_CALIBRATED else 0.35


def classify_path(distance_cm: int) -> str:
    if distance_cm <= 0:
        return "00"          # no echo
    if distance_cm < OBSTACLE_CM:
        return "01"          # blocked
    return "11"              # clear


def encode_telemetry(alert: bool, survivor: int, distance_cm: int, ack: bool) -> str:
    word = "0"                                   # start marker
    word += "1" if alert else "0"                # mission
    word += {0: "00", 1: "01", 2: "10"}[survivor]  # survivor
    word += classify_path(distance_cm)           # path ahead
    word += "11" if ack else "00"                # command centre
    word += "1"                                  # stop marker
    return word


class DeadReckoning:
    """Mirrors the DeadReckoning class in lib/unicorn_core.

    Position relative to where the rover was put down, integrated from the motion
    it was told to perform. No encoders, so the error only grows - which is why
    every position comes with the radius of its own error disc.
    """

    def __init__(self):
        self.reset()

    def reset(self):
        self.x = 0.0
        self.y = 0.0
        self.heading = 0.0        # radians, 0 = the heading it launched on
        self.path = 0.0
        self.turn_seconds = 0.0
        self.motion = "stop"
        self.last = time.time()

    def set_motion(self, motion: str):
        self.update()
        self.motion = motion

    def update(self):
        now = time.time()
        dt = now - self.last
        self.last = now
        if dt <= 0:
            return
        if self.motion == "forward":
            self.x += math.cos(self.heading) * DRIVE_SPEED_CM_S * dt
            self.y += math.sin(self.heading) * DRIVE_SPEED_CM_S * dt
            self.path += DRIVE_SPEED_CM_S * dt
        elif self.motion == "back":
            self.x -= math.cos(self.heading) * DRIVE_SPEED_CM_S * dt
            self.y -= math.sin(self.heading) * DRIVE_SPEED_CM_S * dt
            self.path += DRIVE_SPEED_CM_S * dt
        elif self.motion == "left":
            self.heading += math.radians(TURN_RATE_DEG_S) * dt
            self.turn_seconds += dt
        elif self.motion == "right":
            self.heading -= math.radians(TURN_RATE_DEG_S) * dt
            self.turn_seconds += dt

    def heading_deg(self) -> float:
        return math.degrees(self.heading) % 360.0

    def uncertainty(self) -> float:
        return DRIFT_FRACTION * self.path + TURN_DRIFT_CM_S * self.turn_seconds


class Rover:
    """A small physical model: distance, microphone envelope, mission state."""

    def __init__(self, voice_delay_s: float):
        self.boot = time.time()
        self.alert = False
        self.survivor = 0        # 0 listening, 1 responsive, 2 silent
        self.ack = False
        self.alert_started = 0.0
        self.voice_delay = voice_delay_s
        self.moving = None
        self.nav = DeadReckoning()

    def uptime_ms(self) -> int:
        return int((time.time() - self.boot) * 1000)

    def distance(self) -> int:
        # Debris drifts in and out of the beam as the rover turns. Occasionally
        # the echo is lost entirely, which is what a real HC-SR04 does.
        t = time.time() - self.boot
        base = 60 + 55 * math.sin(t / 4.0) + 12 * math.sin(t / 1.3)
        if self.moving == "forward":
            base -= 25
        if random.random() < 0.03:
            return 0             # echo timed out
        return max(0, int(base))

    def mic_level(self) -> int:
        # Peak-to-peak counts over a window, the same quantity the firmware
        # reports - not an absolute ADC sample.
        floor = random.randint(40, 130)
        if not self.alert:
            return floor
        elapsed = time.time() - self.alert_started
        if elapsed >= self.voice_delay:
            # a voice: a large swing, wavering the way a real one does
            return int(1500 + 500 * math.sin(elapsed * 3)) + floor
        return floor

    def step(self):
        if not self.alert:
            return
        elapsed_ms = (time.time() - self.alert_started) * 1000
        if self.survivor == 0:
            if self.mic_level() >= MIC_THRESHOLD:
                self.survivor = 1
            elif elapsed_ms >= LISTEN_WINDOW_MS:
                self.survivor = 2

    def status(self) -> dict:
        self.step()
        self.nav.update()
        distance = self.distance()
        return {
            "alert": self.alert,
            "survivor": self.survivor,
            "centreAck": self.ack,
            "distance": distance,
            "micLevel": self.mic_level(),
            "micThreshold": MIC_THRESHOLD,
            "word": encode_telemetry(self.alert, self.survivor, distance, self.ack),
            "navX": round(self.nav.x, 1),
            "navY": round(self.nav.y, 1),
            "navHeading": round(self.nav.heading_deg()),
            "navPath": round(self.nav.path),
            "navError": round(self.nav.uncertainty()),
            "navCalibrated": SPEED_CALIBRATED,
            "uptime": self.uptime_ms(),
            "source": "simulator",
        }

    def trigger(self):
        self.alert = True
        self.survivor = 0
        self.ack = False
        self.alert_started = time.time()
        self.moving = None
        self.nav.set_motion("stop")

    def acknowledge(self):
        if self.alert:
            self.ack = True

    def reset(self):
        self.alert = False
        self.survivor = 0
        self.ack = False
        self.moving = None
        self.nav.reset()

    def drive(self, direction: str):
        if self.alert:
            return               # locked during an alert, same as the firmware
        self.moving = None if direction == "stop" else direction
        self.nav.set_motion(direction)


class Handler(BaseHTTPRequestHandler):
    rover: Rover = None

    def log_message(self, *args):
        pass                     # keep the console readable

    def _send(self, code, content_type, body: bytes):
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path.startswith("/status"):
            body = json.dumps(self.rover.status()).encode()
            self._send(200, "application/json", body)
        elif self.path in ("/", "/index.html"):
            self._send(200, "text/html", PANEL.read_bytes())
        else:
            self._send(404, "text/plain", b"not found")

    def do_POST(self):
        path = self.path.rstrip("/")
        if path == "/trigger":
            self.rover.trigger()
        elif path == "/ack":
            self.rover.acknowledge()
        elif path == "/reset":
            self.rover.reset()
        elif path in ("/forward", "/back", "/left", "/right", "/stop"):
            self.rover.drive(path[1:])
        else:
            self._send(404, "text/plain", b"not found")
            return
        self._send(200, "text/plain", b"OK")


def main():
    ap = argparse.ArgumentParser(description="Unicorn rover simulator")
    ap.add_argument("--port", type=int, default=8000)
    ap.add_argument("--voice-delay", type=float, default=6.0,
                    help="seconds before the survivor answers; use a large value "
                         "to exercise the SILENT path")
    args = ap.parse_args()

    if not PANEL.exists():
        raise SystemExit(f"panel not found at {PANEL}")

    Handler.rover = Rover(args.voice_delay)
    server = ThreadingHTTPServer(("0.0.0.0", args.port), Handler)
    print(f"Unicorn simulator (no hardware) -> http://localhost:{args.port}")
    print(f"survivor answers {args.voice_delay:.0f} s after the alert")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nstopped")


if __name__ == "__main__":
    main()
