#include "unicorn_core.h"

#include <math.h>
#include <string.h>

namespace unicorn {

// ---------------------------------------------------------------- obstacle logic
Path classifyPath(int distanceCm) {
  // 0 means the echo never came back inside the timeout: either nothing is in
  // range or the sensor is unhappy. Either way it is not a measurement, so it is
  // reported as Unknown rather than being silently folded into "clear".
  if (distanceCm <= 0) return Path::Unknown;
  if (distanceCm < OBSTACLE_CM) return Path::Blocked;
  return Path::Clear;
}

// ---------------------------------------------------------------- telemetry word
void encodeTelemetry(const Telemetry &t, char *out) {
  out[0] = '0';                       // start marker

  out[1] = t.alert ? '1' : '0';       // mission

  switch (t.survivor) {               // survivor
    case Survivor::Responsive: out[2] = '0'; out[3] = '1'; break;
    case Survivor::Silent:     out[2] = '1'; out[3] = '0'; break;
    default:                   out[2] = '0'; out[3] = '0'; break;
  }

  switch (classifyPath(t.distanceCm)) {  // path ahead
    case Path::Blocked: out[4] = '0'; out[5] = '1'; break;
    case Path::Clear:   out[4] = '1'; out[5] = '1'; break;
    default:            out[4] = '0'; out[5] = '0'; break;
  }

  if (t.centreAck) { out[6] = '1'; out[7] = '1'; }  // command centre
  else             { out[6] = '0'; out[7] = '0'; }

  out[8] = '1';                       // stop marker
  out[9] = '\0';
}

bool validateTelemetryWord(const char *word) {
  if (word == nullptr) return false;
  if (strlen(word) != 9) return false;
  if (word[0] != '0' || word[8] != '1') return false;   // markers

  for (int i = 0; i < 9; i++) {
    if (word[i] != '0' && word[i] != '1') return false;
  }

  // "11" is not a defined survivor code
  if (word[2] == '1' && word[3] == '1') return false;

  // "10" is not a defined path code
  if (word[4] == '1' && word[5] == '0') return false;

  // the centre field is either both set or both clear
  if (word[6] != word[7]) return false;

  return true;
}

// ---------------------------------------------------------------- microphone
MicEnvelope::MicEnvelope(uint32_t windowMs, int threshold)
    : windowMs_(windowMs), threshold_(threshold) {}

void MicEnvelope::reset(uint32_t nowMs) {
  windowStart_ = nowMs;
  min_ = 4095;
  max_ = 0;
  amplitude_ = 0;
  started_ = true;
}

bool MicEnvelope::update(int sample, uint32_t nowMs) {
  if (!started_) {
    reset(nowMs);
  }

  if (sample < min_) min_ = sample;
  if (sample > max_) max_ = sample;

  if (nowMs - windowStart_ < windowMs_) return false;

  amplitude_ = max_ - min_;   // peak-to-peak over the window
  windowStart_ = nowMs;
  min_ = 4095;
  max_ = 0;
  return true;
}

// ---------------------------------------------------------------- dead reckoning
DeadReckoning::DeadReckoning(float speedCmPerS, float turnDegPerS,
                             float driftFraction, float turnDriftCmPerS)
    : speed_(speedCmPerS),
      turnRate_(turnDegPerS * 3.14159265f / 180.0f),
      driftFraction_(driftFraction),
      turnDrift_(turnDriftCmPerS) {}

void DeadReckoning::reset(uint32_t nowMs) {
  x_ = y_ = heading_ = 0;
  path_ = turnSeconds_ = 0;
  motion_ = Motion::Stopped;
  lastUpdate_ = nowMs;
  started_ = true;
}

void DeadReckoning::setMotion(Motion m, uint32_t nowMs) {
  update(nowMs);      // bank what the previous motion earned before switching
  motion_ = m;
}

void DeadReckoning::update(uint32_t nowMs) {
  if (!started_) { reset(nowMs); return; }
  if (nowMs <= lastUpdate_) return;

  float dt = (nowMs - lastUpdate_) / 1000.0f;
  lastUpdate_ = nowMs;

  switch (motion_) {
    case Motion::Forward:
      x_ += cosf(heading_) * speed_ * dt;
      y_ += sinf(heading_) * speed_ * dt;
      path_ += speed_ * dt;
      break;
    case Motion::Back:
      x_ -= cosf(heading_) * speed_ * dt;
      y_ -= sinf(heading_) * speed_ * dt;
      path_ += speed_ * dt;
      break;
    case Motion::TurnLeft:
      heading_ += turnRate_ * dt;
      turnSeconds_ += dt;
      break;
    case Motion::TurnRight:
      heading_ -= turnRate_ * dt;
      turnSeconds_ += dt;
      break;
    case Motion::Stopped:
      break;
  }
}

float DeadReckoning::headingDeg() const {
  float d = heading_ * 180.0f / 3.14159265f;
  while (d < 0) d += 360.0f;
  while (d >= 360.0f) d -= 360.0f;
  return d;
}

float DeadReckoning::uncertaintyCm() const {
  // Straight-line error scales with how far we claim to have gone; turning is
  // worse, because a heading error rotates everything that comes after it.
  return driftFraction_ * path_ + turnDrift_ * turnSeconds_;
}

// ---------------------------------------------------------------- mission clock
Survivor evaluateSurvivor(bool soundDetected, uint32_t msSinceAlert) {
  if (soundDetected) return Survivor::Responsive;
  if (msSinceAlert >= LISTEN_WINDOW_MS) return Survivor::Silent;
  return Survivor::Unknown;
}

}  // namespace unicorn
