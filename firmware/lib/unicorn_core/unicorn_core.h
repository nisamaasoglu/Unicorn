/*
 * unicorn_core - hardware-independent logic
 * -----------------------------------------------------------------------------
 * Everything in this file is plain C++ with no Arduino dependency, so it can be
 * compiled and unit-tested on a PC (`pio test -e native`). The firmware layer in
 * src/main.cpp owns the pins and the Wi-Fi; this layer owns the decisions.
 *
 * Author: Nisa Maasoglu
 * License: MIT
 */

#ifndef UNICORN_CORE_H
#define UNICORN_CORE_H

#include <stdint.h>

namespace unicorn {

// ---------------------------------------------------------------- mission state
enum class Mission : uint8_t {
  Idle,      // charging / standby, operator drives manually
  Alert,     // seismic event declared, siren on, listening for a survivor
  Located    // a survivor has been heard, or the listening window expired
};

enum class Survivor : uint8_t {
  Unknown = 0,   // still listening
  Responsive = 1, // a sound above the threshold was heard
  Silent = 2      // the listening window expired with no sound
};

// ---------------------------------------------------------------- telemetry word
/*
 * The robot reports its state as a fixed-width 9-bit word:
 *
 *   bit 0      start marker, always 0
 *   bit 1      mission: 0 = idle, 1 = alert
 *   bits 2-3   survivor: 00 unknown, 01 responsive, 10 silent
 *   bits 4-5   path ahead: 01 blocked, 11 clear, 00 unknown (no echo)
 *   bits 6-7   command centre: 00 waiting, 11 acknowledged
 *   bit 8      stop marker, always 1
 *
 * Fixed width plus start/stop markers means a receiver can frame the word
 * without a length field, and an all-zero or all-one line fault fails the
 * marker check instead of decoding as a valid state.
 */
struct Telemetry {
  bool alert = false;
  Survivor survivor = Survivor::Unknown;
  int distanceCm = 0;        // 0 = no echo within range
  bool centreAck = false;
};

// Writes 9 characters plus a terminating NUL into `out` (needs 10 bytes).
void encodeTelemetry(const Telemetry &t, char *out);

// Returns true when the word is well-formed (length, markers, known fields).
bool validateTelemetryWord(const char *word);

// ---------------------------------------------------------------- obstacle logic
const int OBSTACLE_CM = 15;   // closer than this counts as blocked

enum class Path : uint8_t { Unknown = 0, Blocked = 1, Clear = 2 };

Path classifyPath(int distanceCm);

// ---------------------------------------------------------------- microphone
/*
 * A MAX9814 outputs an AC audio signal riding on a DC bias of roughly 1.25 V, so
 * a single ADC sample lands at an arbitrary point on the waveform - it may catch
 * a peak or a zero crossing of the very same sound. Thresholding one sample is a
 * coin flip.
 *
 * This tracks the peak-to-peak amplitude across a time window instead, which is
 * what actually corresponds to loudness.
 */
class MicEnvelope {
 public:
  explicit MicEnvelope(uint32_t windowMs = 50, int threshold = 900);

  // Feed every sample. Returns true once per window, when a result is ready.
  bool update(int sample, uint32_t nowMs);

  int amplitude() const { return amplitude_; }        // last completed window
  bool detected() const { return amplitude_ >= threshold_; }
  int threshold() const { return threshold_; }
  void setThreshold(int t) { threshold_ = t; }
  void reset(uint32_t nowMs);

 private:
  uint32_t windowMs_;
  int threshold_;
  uint32_t windowStart_ = 0;
  int min_ = 4095;
  int max_ = 0;
  int amplitude_ = 0;
  bool started_ = false;
};

// ---------------------------------------------------------------- dead reckoning
/*
 * The rover carries no GNSS receiver - an ESP32 has no such radio. What it can do
 * is estimate how far it has moved from the point it was deployed, by integrating
 * the motion it was commanded to perform. That is dead reckoning: position
 * relative to a known origin, not an absolute fix.
 *
 * There are no wheel encoders on this rover, so the estimate rests on a nominal
 * speed rather than a measurement of the wheels actually turning. A stalled
 * motor, a slipping wheel or a low battery all make the rover travel less than
 * this model believes. The error therefore only ever grows, and the class
 * reports that growth rather than hiding it: read `uncertaintyCm()` next to
 * every position it gives you.
 *
 * The origin stays with the operator. The rover reports displacement in
 * centimetres in its own local frame; the command centre knows where it put the
 * robot down. That way nothing here has to pretend to be a coordinate.
 */
enum class Motion : uint8_t { Stopped, Forward, Back, TurnLeft, TurnRight };

class DeadReckoning {
 public:
  DeadReckoning(float speedCmPerS, float turnDegPerS,
                float driftFraction, float turnDriftCmPerS);

  void setMotion(Motion m, uint32_t nowMs);
  void update(uint32_t nowMs);
  void reset(uint32_t nowMs);

  float x() const { return x_; }                    // cm, +x is the launch heading
  float y() const { return y_; }                    // cm
  float headingDeg() const;                         // 0-360, 0 = launch heading
  float pathLengthCm() const { return path_; }      // total ground covered
  float uncertaintyCm() const;                      // radius of the error disc
  Motion motion() const { return motion_; }

 private:
  float speed_, turnRate_, driftFraction_, turnDrift_;
  float x_ = 0, y_ = 0, heading_ = 0;   // heading in radians
  float path_ = 0, turnSeconds_ = 0;
  Motion motion_ = Motion::Stopped;
  uint32_t lastUpdate_ = 0;
  bool started_ = false;
};

// ---------------------------------------------------------------- mission clock
/*
 * How long the robot listens before it declares the survivor unresponsive.
 * Kept here (not in the firmware) so the timing is covered by the tests.
 */
const uint32_t LISTEN_WINDOW_MS = 15000;

Survivor evaluateSurvivor(bool soundDetected, uint32_t msSinceAlert);

}  // namespace unicorn

#endif  // UNICORN_CORE_H
