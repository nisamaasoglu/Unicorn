/* Minimal WebServer stub - see Arduino.h for why. License: MIT */
#ifndef WEBSERVER_H_STUB
#define WEBSERVER_H_STUB
#include <functional>
#include "Arduino.h"

#define HTTP_POST 1

class WebServer {
 public:
  explicit WebServer(int port);
  void on(const char* uri, std::function<void()> handler);
  void on(const char* uri, int method, std::function<void()> handler);
  void send(int code, const char* contentType, const String& content);
  void send_P(int code, PGM_P contentType, PGM_P content);
  void begin();
  void handleClient();
};
#endif
