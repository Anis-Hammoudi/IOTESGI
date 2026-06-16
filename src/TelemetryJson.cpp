#include "TelemetryJson.h"

#include <ArduinoJson.h>

#include "AppConfig.h"

String buildTelemetryJson(const ReactorState& state) {
  DynamicJsonDocument doc(512);
  
  // Format imposé par la consigne :
  // { "device": "ESP32-X", "ts": 0, "data": { ... } }
  doc["device"] = String("ESP32-") + AppConfig::DeviceId;
  doc["ts"] = state.timestampMs;

  JsonObject data = doc.createNestedObject("data");
  
  if (!isnan(state.roomTemperatureC)) {
    data["temp"] = state.roomTemperatureC;
  }
  if (!isnan(state.roomHumidityPercent)) {
    data["humidity"] = state.roomHumidityPercent;
  }
  
  // Données étendues
  if (!isnan(state.coreTemperatureC)) {
    data["coreRadiationC"] = state.coreTemperatureC;
  }
  data["lightRaw"] = state.lightRaw;
  data["status"] = reactorStatusToString(state.status);
  data["rodPositionPercent"] = state.rodPositionPercent;
  data["emergencyStop"] = state.emergencyStop;
  data["online"] = state.reactorOnline;
  data["alarmAcknowledged"] = state.alarmAcknowledged;
  data["wifiConnected"] = state.wifiConnected;
  data["mqttConnected"] = state.mqttConnected;
  data["freeHeap"] = state.freeHeap;
  data["uptimeMs"] = state.uptimeMs;
  data["offlineQueueSize"] = state.offlineQueueSize;

  String output;
  serializeJson(doc, output);
  return output;
}

bool validateAndApplyCommand(const String& payload, ReactorState& state) {
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    return false;
  }

  const String command = doc["command"] | "";
  if (command == "ACK_ALARM") {
    state.alarmAcknowledged = true;
    return true;
  }

  if (command == "EMERGENCY_STOP") {
    state.emergencyStop = true;
    state.alarmAcknowledged = true;
    state.rodPositionPercent = 100;
    return true;
  }

  if (command == "RESET_EMERGENCY") {
    state.emergencyStop = false;
    state.alarmAcknowledged = true;
    return true;
  }

  if (command == "SET_ROD") {
    if (!doc["value"].is<int>()) {
      return false;
    }
    state.rodPositionPercent = constrain(doc["value"].as<int>(), 0, 100);
    return true;
  }

  return false;
}
