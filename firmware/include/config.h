/*
 * Pin map and tuning constants for the Unicorn rover.
 * -----------------------------------------------------------------------------
 * Every pin the firmware touches is declared here and nowhere else, so a wiring
 * change is a one-file change and a conflict is visible at a glance.
 *
 * ESP32 strapping pins - avoided on purpose:
 *   GPIO0, GPIO2   boot mode selection
 *   GPIO12 (MTDI)  sets the flash voltage at reset; if it is held HIGH at boot
 *                  the ESP32 selects 1.8 V flash and the board does not start.
 *                  An L298N input pin idles HIGH, which is exactly this fault.
 *   GPIO15 (MTDO)  suppresses the boot log when pulled low
 *
 * None of these carries a motor or a sensor here.
 *
 * Input-only pins (no output, no pull-ups): GPIO34-39. GPIO34 suits the
 * microphone, which is read-only.
 *
 * Author: Nisa Maasoglu
 * License: MIT
 */

#ifndef CONFIG_H
#define CONFIG_H

// ---------------------------------------------------------------- indicators
// A 5 mm common-cathode RGB LED. Green is unused by the mission states, so it
// is not wired. Red + blue together read as magenta, which marks the idle state.
#define LED_RED_PIN 4
#define LED_BLUE_PIN 17

#define BUZZER_PIN 33          // 8R 0.5W speaker through a series resistor

// ---------------------------------------------------------------- sensing
#define MIC_PIN 34             // MAX9814 OUT - input-only pin, correct for ADC
#define TRIG_PIN 5             // HC-SR04 trigger  (was GPIO2: strapping pin)
#define ECHO_PIN 18            // HC-SR04 echo     (was GPIO15: strapping pin)

// ---------------------------------------------------------------- drive
// L298N dual H-bridge, four 6 V 250 RPM motors wired as two sides.
#define MOTOR_LEFT_FWD 26      // was GPIO12: strapping pin, blocked booting
#define MOTOR_LEFT_REV 13
#define MOTOR_RIGHT_FWD 25
#define MOTOR_RIGHT_REV 27

// ---------------------------------------------------------------- network
// The rover is its own access point: in a disaster scenario there is no router
// to join, so the command centre connects to the robot rather than the reverse.
#define AP_SSID "UNICORN_MERKEZ"
#define AP_PASSWORD "unicorn123"     // WPA2 needs 8 characters minimum

// ---------------------------------------------------------------- timing
#define SERIAL_BAUD 115200
#define DISTANCE_INTERVAL_MS 100     // ultrasonic ping rate
#define ECHO_TIMEOUT_US 25000UL      // ~4 m; pulseIn blocks for at most this long
#define MIC_WINDOW_MS 50             // peak-to-peak window for the microphone
#define MIC_THRESHOLD 900            // peak-to-peak counts that count as a voice
#define SIREN_HALF_PERIOD_MS 500     // on/off cadence of the alert tone
#define SIREN_TONE_HZ 1200
#define STARTUP_SWEEP_MS 4000        // opening orientation turn on power-up

// ---------------------------------------------------------------- dead reckoning
/*
 * There are no wheel encoders, so position is integrated from a nominal speed
 * rather than from the wheels actually turning. Two numbers are the whole model.
 *
 * Where the defaults come from:
 *   two 18650s in series feed the L298N about 7.4 V, and the bridge drops
 *   roughly 2 V, so the 6 V 250 RPM gearmotors see about 5.4 V
 *     -> 250 x (5.4 / 6)            = 225 rpm  = 3.75 rev/s
 *   the yellow TT wheels measure 65 mm across
 *     -> pi x 6.5 cm                = 20.4 cm per revolution
 *     -> 3.75 x 20.4                = 76 cm/s unloaded
 *   a loaded gearmotor turns slower than its no-load figure, so roughly
 *     -> 76 x 0.63                  = 48 cm/s
 *
 * That last step is an estimate, not a measurement, and the chassis is foam
 * board on a hard floor - carpet or half-flat cells make it lower still.
 *
 * SPEED_CALIBRATED tells the rover whether anyone has checked. While it is 0 the
 * position is still reported, but the error radius grows more than twice as fast
 * and the panel says the model is uncalibrated. A robot that knows how much to
 * trust itself is worth more than one that quietly reports a confident number.
 *
 * To calibrate, on the surface you will actually drive on:
 *   speed  - hold FORWARD for 10 s, measure the distance covered, divide by 10
 *   turn   - hold LEFT until the rover returns to its starting heading, time it,
 *            and divide 360 by that
 * then set the two values below and flip SPEED_CALIBRATED to 1.
 */
#define SPEED_CALIBRATED 0           // 1 once the two figures below are measured

#define DRIVE_SPEED_CM_S 48.0f       // derived above; measure it and correct it
#define TURN_RATE_DEG_S 120.0f       // skid-steer, both sides opposed
#define TURN_DRIFT_CM_S 3.0f         // extra error per second of turning

#if SPEED_CALIBRATED
  #define DRIFT_FRACTION 0.15f       // measured model: error is ~15% of distance
#else
  #define DRIFT_FRACTION 0.35f       // derived model: trust it far less
#endif

#endif  // CONFIG_H
