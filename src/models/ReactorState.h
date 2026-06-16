#pragma once

#include <Arduino.h>

enum class ReactorStatus : uint8_t {
  Offline,
  Nominal,
  Warning,
  Critical,
  EmergencyStop
};

struct ReactorState {
  uint32_t timestampMs = 0;
  uint16_t lightRaw = 0;
  bool reactorOnline = false;
  float coreTemperatureC = NAN;
  float roomTemperatureC = NAN;
  float roomHumidityPercent = NAN;
  int rodPositionPercent = 0;
  bool emergencyStop = false;
  bool alarmAcknowledged = false;
  ReactorStatus status = ReactorStatus::Offline;
  bool wifiConnected = false;
  bool mqttConnected = false;
  uint32_t freeHeap = 0;
  uint32_t uptimeMs = 0;
  size_t offlineQueueSize = 0;
};

inline const char* reactorStatusToString(ReactorStatus status) {
  switch (status) {
    case ReactorStatus::Offline:
      return "OFFLINE";
    case ReactorStatus::Nominal:
      return "NOMINAL";
    case ReactorStatus::Warning:
      return "WARNING";
    case ReactorStatus::Critical:
      return "CRITICAL";
    case ReactorStatus::EmergencyStop:
      return "EMERGENCY_STOP";
  }
  return "UNKNOWN";
}
