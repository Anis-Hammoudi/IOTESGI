#pragma once

#include <Arduino.h>

class ConnectivityManager {
 public:
  void begin();
  void loop();
  bool wifiConnected() const;

 private:
  uint32_t lastAttemptMs_ = 0;
};
