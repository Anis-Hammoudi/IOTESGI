#include "ConnectivityManager.h"

#include <WiFi.h>

#include "AppConfig.h"

namespace {
constexpr uint32_t WifiRetryMs = 10000;
}

void ConnectivityManager::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(AppConfig::WifiSsid, AppConfig::WifiPassword);
  lastAttemptMs_ = millis();
}

void ConnectivityManager::loop() {
  if (wifiConnected()) {
    return;
  }

  const uint32_t now = millis();
  if (now - lastAttemptMs_ >= WifiRetryMs) {
    WiFi.disconnect();
    WiFi.begin(AppConfig::WifiSsid, AppConfig::WifiPassword);
    lastAttemptMs_ = now;
  }
}

bool ConnectivityManager::wifiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}
