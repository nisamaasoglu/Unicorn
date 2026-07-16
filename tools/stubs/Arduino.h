/*
 * Minimal Arduino API stubs for the host-side compile check.
 *
 * These declare the signatures the firmware actually calls, so a host compiler
 * can type-check src/main.cpp without the ESP32 toolchain. They implement
 * nothing and they are never flashed.
 *
 * This catches syntax errors, typos, wrong argument counts and type mismatches.
 * It is not a substitute for building against the real Arduino ESP32 core -
 * run `pio run -e fm-devkit` before you trust the binary.
 *
 * License: MIT
 */
#ifndef ARDUINO_H_STUB
#define ARDUINO_H_STUB

#include <stdint.h>
#include <string>

#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0
#define PROGMEM
#define F(x) (x)
typedef const char* PGM_P;

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
int analogRead(uint8_t pin);
unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);
unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout);
void tone(uint8_t pin, unsigned int frequency);
void noTone(uint8_t pin);

// Arduino's String, reduced to what the firmware uses.
class String {
 public:
  String() {}
  String(const char* s) : s_(s) {}
  String(int v);
  String(unsigned long v);
  String(float v, int decimalPlaces = 2);
  String& operator+=(const String& o) { s_ += o.s_; return *this; }
  String& operator+=(const char* o) { s_ += o; return *this; }
  const char* c_str() const { return s_.c_str(); }
  operator const char*() const { return s_.c_str(); }
 private:
  std::string s_;
};

// The real core prints anything deriving from Printable, which is how
// Serial.println(WiFi.softAPIP()) resolves. Mirrored here so the stub accepts
// exactly what the core accepts, and no more.
class Printable {
 public:
  virtual ~Printable() {}
};

class SerialClass {
 public:
  void begin(unsigned long baud);
  void print(const char* s);
  void println(const char* s);
  void print(int v);
  void println(int v);
  void print(const Printable& p);
  void println(const Printable& p);
};
extern SerialClass Serial;

#endif
