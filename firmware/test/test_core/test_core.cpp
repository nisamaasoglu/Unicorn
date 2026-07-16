/*
 * Native unit tests for unicorn_core.
 *
 * Run on a PC, no ESP32 required:
 *     cd firmware && pio test -e native
 *
 * Uses a minimal hand-rolled harness (platformio.ini sets test_framework=custom)
 * so the same file also compiles with a bare g++ call.
 *
 * Author: Nisa Maasoglu
 * License: MIT
 */

#include <cstdio>
#include <cstring>
#include <string>

#include "unicorn_core.h"

using namespace unicorn;

static int checks = 0;
static int failures = 0;

static void check(bool condition, const std::string &name) {
  checks++;
  if (condition) {
    printf("  ok   %s\n", name.c_str());
  } else {
    failures++;
    printf("  FAIL %s\n", name.c_str());
  }
}

static std::string encode(bool alert, Survivor s, int dist, bool ack) {
  Telemetry t;
  t.alert = alert;
  t.survivor = s;
  t.distanceCm = dist;
  t.centreAck = ack;
  char out[10];
  encodeTelemetry(t, out);
  return std::string(out);
}

// ---------------------------------------------------------------- telemetry word
static void test_telemetry_word() {
  printf("telemetry word\n");

  check(encode(false, Survivor::Unknown, 0, false).size() == 9,
        "word is exactly 9 bits wide");

  check(encode(false, Survivor::Unknown, 0, false)[0] == '0',
        "start marker is always 0");

  check(encode(true, Survivor::Responsive, 40, true)[8] == '1',
        "stop marker is always 1");

  // idle, nothing heard, path clear, no ack
  check(encode(false, Survivor::Unknown, 100, false) == "000011001",
        "idle + clear path + no ack encodes as 000011001");

  // alert, survivor responsive, path blocked, centre acknowledged
  check(encode(true, Survivor::Responsive, 10, true) == "010101111",
        "alert + responsive + blocked + ack encodes as 010101111");

  // alert, survivor silent, no echo, waiting
  check(encode(true, Survivor::Silent, 0, false) == "011000001",
        "alert + silent + no echo + waiting encodes as 011000001");

  check(encode(true, Survivor::Responsive, 10, true) !=
            encode(true, Survivor::Responsive, 10, false),
        "the ack field actually changes the word");
}

static void test_word_validation() {
  printf("word validation\n");

  check(validateTelemetryWord(encode(true, Survivor::Responsive, 40, true).c_str()),
        "a freshly encoded word validates");

  check(!validateTelemetryWord("000000000"),
        "an all-zero line fault is rejected (stop marker missing)");

  check(!validateTelemetryWord("111111111"),
        "an all-one line fault is rejected (start marker missing)");

  check(!validateTelemetryWord("01011011"),
        "a truncated 8-bit word is rejected");

  check(!validateTelemetryWord("0111100 1"),
        "a word with a stray character is rejected");

  check(!validateTelemetryWord("011100001"),
        "survivor code 11 is undefined and rejected");

  check(!validateTelemetryWord("010110001"),
        "path code 10 is undefined and rejected");

  check(!validateTelemetryWord("010101101"),
        "a half-set centre field is rejected");

  check(!validateTelemetryWord(nullptr), "a null pointer is rejected");
}

// ---------------------------------------------------------------- obstacle logic
static void test_path() {
  printf("path classification\n");

  check(classifyPath(0) == Path::Unknown,
        "no echo is Unknown, not silently treated as clear");
  check(classifyPath(-3) == Path::Unknown, "a negative reading is Unknown");
  check(classifyPath(14) == Path::Blocked, "14 cm is blocked");
  check(classifyPath(15) == Path::Clear, "15 cm is exactly at the clear boundary");
  check(classifyPath(200) == Path::Clear, "200 cm is clear");
}

// ---------------------------------------------------------------- microphone
static void test_mic_envelope() {
  printf("microphone envelope\n");

  // A silent MAX9814 still sits at its DC bias (~1550 counts on a 12-bit ADC).
  // The old single-sample threshold of >2500 could never see this correctly:
  // it is the swing that carries the loudness, not the absolute level.
  MicEnvelope quiet(50, 900);
  uint32_t t = 0;
  int completed = 0;
  for (int i = 0; i <= 100; i++) {        // t = 0..100 ms inclusive
    int sample = 1550 + (i % 2 ? 8 : -8);   // tiny noise around the bias
    if (quiet.update(sample, t)) completed++;
    t += 1;
  }
  check(completed == 2, "a 100 ms run completes exactly two 50 ms windows");
  check(quiet.amplitude() < 100, "a quiet room yields a small peak-to-peak swing");
  check(!quiet.detected(), "a quiet room does not trigger a detection");

  // A shout: a large swing around the same bias.
  MicEnvelope loud(50, 900);
  t = 0;
  for (int i = 0; i < 100; i++) {
    int sample = 1550 + (i % 2 ? 700 : -700);
    loud.update(sample, t);
    t += 1;
  }
  check(loud.amplitude() >= 1400, "a shout yields a large peak-to-peak swing");
  check(loud.detected(), "a shout triggers a detection");

  // The decisive case: a loud sound sampled at a zero crossing sits at the bias.
  // A single-sample threshold would call this silence; the envelope does not.
  MicEnvelope crossing(50, 900);
  t = 0;
  int pattern[4] = {1550, 2250, 1550, 850};   // sine-like, crosses the bias twice
  for (int i = 0; i < 100; i++) {
    crossing.update(pattern[i % 4], t);
    t += 1;
  }
  check(crossing.detected(),
        "a loud sound is still detected even though many samples sit at the bias");

  MicEnvelope r(50, 900);
  r.update(4000, 0);
  r.reset(0);
  check(r.amplitude() == 0, "reset clears the amplitude");
}

// ---------------------------------------------------------------- mission clock
static void test_survivor_evaluation() {
  printf("survivor evaluation\n");

  check(evaluateSurvivor(true, 10) == Survivor::Responsive,
        "sound at any time makes the survivor Responsive");
  check(evaluateSurvivor(false, 10) == Survivor::Unknown,
        "silence inside the window keeps the survivor Unknown");
  check(evaluateSurvivor(false, LISTEN_WINDOW_MS - 1) == Survivor::Unknown,
        "one millisecond before the window closes it is still Unknown");
  check(evaluateSurvivor(false, LISTEN_WINDOW_MS) == Survivor::Silent,
        "silence for the whole window makes the survivor Silent");
  check(evaluateSurvivor(true, LISTEN_WINDOW_MS + 5000) == Survivor::Responsive,
        "a late sound still counts as Responsive");
}

// ---------------------------------------------------------------- dead reckoning
static void test_dead_reckoning() {
  printf("dead reckoning\n");

  // 20 cm/s forward, 90 deg/s turning, 15% drift, 3 cm per second of turning
  DeadReckoning dr(20.0f, 90.0f, 0.15f, 3.0f);
  dr.reset(0);

  check(dr.x() == 0 && dr.y() == 0, "starts at the origin");
  check(dr.uncertaintyCm() == 0, "starts with no accumulated error");

  dr.setMotion(Motion::Forward, 0);
  dr.update(1000);                         // one second forward
  check(dr.x() > 19.0f && dr.x() < 21.0f, "1 s forward at 20 cm/s advances ~20 cm");
  check(dr.y() > -0.01f && dr.y() < 0.01f, "driving straight does not drift sideways");
  check(dr.pathLengthCm() > 19.0f, "path length tracks the ground covered");

  dr.setMotion(Motion::Stopped, 1000);
  dr.update(5000);                         // four seconds parked
  check(dr.x() > 19.0f && dr.x() < 21.0f, "a parked rover does not accumulate distance");

  // turn 90 degrees left, then drive: the displacement must land on the y axis
  DeadReckoning t(20.0f, 90.0f, 0.15f, 3.0f);
  t.reset(0);
  t.setMotion(Motion::TurnLeft, 0);
  t.update(1000);                          // 90 deg/s for 1 s
  check(t.headingDeg() > 89.0f && t.headingDeg() < 91.0f, "1 s of turning is ~90 degrees");
  t.setMotion(Motion::Forward, 1000);
  t.update(2000);
  check(t.y() > 19.0f && t.y() < 21.0f, "after a left turn the rover advances along +y");
  check(t.x() < 0.5f, "after a left turn it stops advancing along +x");

  // heading must stay inside 0-360 however far it spins
  DeadReckoning spin(20.0f, 90.0f, 0.15f, 3.0f);
  spin.reset(0);
  spin.setMotion(Motion::TurnRight, 0);
  spin.update(10000);                      // 900 degrees clockwise
  check(spin.headingDeg() >= 0.0f && spin.headingDeg() < 360.0f,
        "heading is normalised after multiple full turns");

  // the error disc only ever grows, and it grows faster while turning
  DeadReckoning e(20.0f, 90.0f, 0.15f, 3.0f);
  e.reset(0);
  e.setMotion(Motion::Forward, 0);
  e.update(1000);
  float afterDriving = e.uncertaintyCm();
  check(afterDriving > 2.9f && afterDriving < 3.1f, "20 cm driven -> ~3 cm of error");
  e.setMotion(Motion::TurnLeft, 1000);
  e.update(2000);
  check(e.uncertaintyCm() > afterDriving, "turning adds error even though x and y hold");
  check(e.pathLengthCm() > 19.0f && e.pathLengthCm() < 21.0f,
        "turning on the spot adds no ground distance");

  // An unmeasured speed is a worse speed, and the error model has to say so.
  DeadReckoning derived(20.0f, 90.0f, 0.35f, 3.0f);   // SPEED_CALIBRATED = 0
  DeadReckoning measured(20.0f, 90.0f, 0.15f, 3.0f);  // SPEED_CALIBRATED = 1
  derived.reset(0); measured.reset(0);
  derived.setMotion(Motion::Forward, 0); measured.setMotion(Motion::Forward, 0);
  derived.update(5000); measured.update(5000);
  check(derived.x() > 99.0f && measured.x() > 99.0f,
        "both models report the same position");
  check(derived.uncertaintyCm() > measured.uncertaintyCm() * 2.0f,
        "an uncalibrated model reports more than twice the error for the same drive");
}

int main() {
  printf("\nunicorn_core tests\n==================\n");
  test_telemetry_word();
  test_word_validation();
  test_path();
  test_mic_envelope();
  test_survivor_evaluation();
  test_dead_reckoning();
  printf("==================\n%d checks, %d failures\n\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
