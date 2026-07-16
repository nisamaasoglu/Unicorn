/* Minimal WiFi stub - see Arduino.h for why. License: MIT */
#ifndef WIFI_H_STUB
#define WIFI_H_STUB
#include "Arduino.h"

class IPAddress : public Printable {
 public:
  const char* toString() const { return "192.168.4.1"; }
};

class WiFiClass {
 public:
  bool softAP(const char* ssid, const char* password);
  IPAddress softAPIP();
};
extern WiFiClass WiFi;
#endif
