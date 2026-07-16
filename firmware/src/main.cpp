/*
 * Unicorn - ESP32 firmware
 * -----------------------------------------------------------------------------
 * A four-wheeled search-and-rescue rover. The ESP32 raises its own Wi-Fi access
 * point and serves a command-centre page; an operator drives the rover, declares
 * a seismic event, and acknowledges the rover once help is on the way. The rover
 * listens for a survivor through a MAX9814 microphone and reports its state as a
 * 9-bit telemetry word.
 *
 * The mission logic itself lives in lib/unicorn_core, which has no Arduino
 * dependency and is unit-tested on a PC. This file owns only the hardware.
 *
 * Endpoints
 *   GET  /          command centre page (embedded in PROGMEM)
 *   GET  /status    JSON telemetry
 *   POST /trigger   declare a seismic event
 *   POST /ack       command centre acknowledges the rover
 *   POST /reset     return to idle
 *   POST /forward /back /left /right /stop   manual drive
 *
 * Author: Nisa Maasoglu
 * License: MIT
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "config.h"
#include "unicorn_core.h"
#include "web_page.h"

using namespace unicorn;

WebServer server(80);

// ---------------------------------------------------------------- state
Mission mission = Mission::Idle;
Survivor survivor = Survivor::Unknown;
bool centreAck = false;

int distanceCm = 0;
MicEnvelope mic(MIC_WINDOW_MS, MIC_THRESHOLD);
DeadReckoning nav(DRIVE_SPEED_CM_S, TURN_RATE_DEG_S, DRIFT_FRACTION, TURN_DRIFT_CM_S);

unsigned long alertStartedAt = 0;
unsigned long lastDistanceAt = 0;
unsigned long bootAt = 0;
bool startupSweepDone = false;

// ---------------------------------------------------------------- drive
// Each of these is the single place where a motion both reaches the H-bridge and
// reaches the position estimate, so the two cannot drift out of agreement.
void driveStop() {
  digitalWrite(MOTOR_LEFT_FWD, LOW);  digitalWrite(MOTOR_LEFT_REV, LOW);
  digitalWrite(MOTOR_RIGHT_FWD, LOW); digitalWrite(MOTOR_RIGHT_REV, LOW);
  nav.setMotion(Motion::Stopped, millis());
}
void driveForward() {
  digitalWrite(MOTOR_LEFT_FWD, HIGH); digitalWrite(MOTOR_RIGHT_FWD, HIGH);
  digitalWrite(MOTOR_LEFT_REV, LOW);  digitalWrite(MOTOR_RIGHT_REV, LOW);
  nav.setMotion(Motion::Forward, millis());
}
void driveBack() {
  digitalWrite(MOTOR_LEFT_REV, HIGH); digitalWrite(MOTOR_RIGHT_REV, HIGH);
  digitalWrite(MOTOR_LEFT_FWD, LOW);  digitalWrite(MOTOR_RIGHT_FWD, LOW);
  nav.setMotion(Motion::Back, millis());
}
void driveLeft() {
  digitalWrite(MOTOR_LEFT_REV, HIGH); digitalWrite(MOTOR_RIGHT_FWD, HIGH);
  digitalWrite(MOTOR_LEFT_FWD, LOW);  digitalWrite(MOTOR_RIGHT_REV, LOW);
  nav.setMotion(Motion::TurnLeft, millis());
}
void driveRight() {
  digitalWrite(MOTOR_LEFT_FWD, HIGH); digitalWrite(MOTOR_RIGHT_REV, HIGH);
  digitalWrite(MOTOR_LEFT_REV, LOW);  digitalWrite(MOTOR_RIGHT_FWD, LOW);
  nav.setMotion(Motion::TurnRight, millis());
}

// ---------------------------------------------------------------- indicators
void setIndicators() {
  switch (mission) {
    case Mission::Idle:
      // red + blue together = magenta: powered, charging, waiting
      digitalWrite(LED_RED_PIN, HIGH);
      digitalWrite(LED_BLUE_PIN, HIGH);
      break;
    case Mission::Alert:
    case Mission::Located:
      if (centreAck) {                    // help is on the way
        digitalWrite(LED_RED_PIN, LOW);
        digitalWrite(LED_BLUE_PIN, HIGH);
      } else {                            // emergency, nobody has answered yet
        digitalWrite(LED_RED_PIN, HIGH);
        digitalWrite(LED_BLUE_PIN, LOW);
      }
      break;
  }
}

void updateSiren() {
  // The siren runs only while the rover is still trying to raise a response.
  // Once the survivor has been heard, silence is more useful than noise: it lets
  // them hear the rescuers, and it stops the speaker masking the microphone.
  bool sirenWanted = (mission == Mission::Alert) &&
                     (survivor == Survivor::Unknown) && !centreAck;

  if (!sirenWanted) {
    noTone(BUZZER_PIN);
    return;
  }

  if ((millis() / SIREN_HALF_PERIOD_MS) % 2 == 0) tone(BUZZER_PIN, SIREN_TONE_HZ);
  else noTone(BUZZER_PIN);
}

// ---------------------------------------------------------------- sensing
void measureDistance() {
  if (millis() - lastDistanceAt < DISTANCE_INTERVAL_MS) return;
  lastDistanceAt = millis();

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // pulseIn blocks; the timeout caps that at ~25 ms and only 10 times a second,
  // instead of on every pass of the loop where it starved the web server.
  unsigned long echoUs = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  distanceCm = (echoUs == 0) ? 0 : (int)(echoUs * 0.034 / 2);
}

void listen() {
  if (mission != Mission::Alert) return;

  // Sampled on every pass; the envelope decides once per window.
  if (!mic.update(analogRead(MIC_PIN), millis())) return;

  Survivor next = evaluateSurvivor(mic.detected(), millis() - alertStartedAt);
  if (next != Survivor::Unknown) {
    survivor = next;
    mission = Mission::Located;
  }
}

// ---------------------------------------------------------------- HTTP
void handleRoot() {
  server.send_P(200, "text/html", COMMAND_CENTRE_HTML);
}

void handleStatus() {
  Telemetry t;
  t.alert = (mission != Mission::Idle);
  t.survivor = survivor;
  t.distanceCm = distanceCm;
  t.centreAck = centreAck;

  char word[10];
  encodeTelemetry(t, word);

  String json = "{";
  json += "\"alert\":";      json += (t.alert ? "true" : "false");
  json += ",\"survivor\":";  json += String((int)survivor);
  json += ",\"centreAck\":"; json += (centreAck ? "true" : "false");
  json += ",\"distance\":";  json += String(distanceCm);
  json += ",\"micLevel\":";  json += String(mic.amplitude());
  json += ",\"micThreshold\":"; json += String(mic.threshold());
  json += ",\"word\":\"";    json += word; json += "\"";
  json += ",\"navX\":";      json += String(nav.x(), 1);
  json += ",\"navY\":";      json += String(nav.y(), 1);
  json += ",\"navHeading\":"; json += String(nav.headingDeg(), 0);
  json += ",\"navPath\":";   json += String(nav.pathLengthCm(), 0);
  json += ",\"navError\":";  json += String(nav.uncertaintyCm(), 0);
  json += ",\"navCalibrated\":"; json += (SPEED_CALIBRATED ? "true" : "false");
  json += ",\"uptime\":";    json += String(millis());
  json += ",\"source\":\"device\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handleTrigger() {
  mission = Mission::Alert;
  survivor = Survivor::Unknown;
  centreAck = false;
  alertStartedAt = millis();
  mic.reset(millis());
  driveStop();
  server.send(200, "text/plain", "OK");
}

void handleAck() {
  if (mission != Mission::Idle) centreAck = true;
  server.send(200, "text/plain", "OK");
}

void handleReset() {
  // An alert has to be reversible. A rescue rover that can only be armed once,
  // and released only by its power switch, is not a rover.
  mission = Mission::Idle;
  survivor = Survivor::Unknown;
  centreAck = false;
  noTone(BUZZER_PIN);
  driveStop();
  nav.reset(millis());     // a reset re-declares where the rover is standing
  server.send(200, "text/plain", "OK");
}

// Manual driving is blocked during an alert: the rover holds position so the
// microphone is not fighting four gearmotors while it listens.
void handleDrive(void (*action)()) {
  if (mission == Mission::Idle) action();
  server.send(200, "text/plain", "OK");
}

// ---------------------------------------------------------------- setup / loop
void setup() {
  Serial.begin(SERIAL_BAUD);
  bootAt = millis();

  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  // MIC_PIN is GPIO34, input-only: no pinMode needed and none is possible.

  pinMode(MOTOR_LEFT_FWD, OUTPUT);  pinMode(MOTOR_LEFT_REV, OUTPUT);
  pinMode(MOTOR_RIGHT_FWD, OUTPUT); pinMode(MOTOR_RIGHT_REV, OUTPUT);
  driveStop();

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print(F("Command centre at http://"));
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/trigger", HTTP_POST, handleTrigger);
  server.on("/ack", HTTP_POST, handleAck);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/forward", HTTP_POST, []() { handleDrive(driveForward); });
  server.on("/back",    HTTP_POST, []() { handleDrive(driveBack); });
  server.on("/left",    HTTP_POST, []() { handleDrive(driveLeft); });
  server.on("/right",   HTTP_POST, []() { handleDrive(driveRight); });
  server.on("/stop",    HTTP_POST, []() { handleDrive(driveStop); });
  server.begin();

  mic.reset(millis());
  nav.reset(millis());
}

void loop() {
  server.handleClient();

  nav.update(millis());
  measureDistance();
  listen();
  setIndicators();
  updateSiren();

  // Opening orientation turn, once, on power-up. Written against millis() rather
  // than delay() so the access point answers from the first second.
  if (!startupSweepDone && mission == Mission::Idle) {
    if (millis() - bootAt < STARTUP_SWEEP_MS) {
      driveLeft();
    } else {
      driveStop();
      startupSweepDone = true;
    }
  }
}
