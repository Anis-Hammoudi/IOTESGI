#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <functional>

#include "config/RuntimeConfig.h"
#include "models/ReactorState.h"

class WebServerManager {
 public:
  using StateProvider = std::function<ReactorState()>;
  using TelemetryProvider = std::function<String()>;
  using ConfigSaver = std::function<bool(const MqttSettings&)>;
  using CommandHandler = std::function<void(const String&)>;

  WebServerManager();
  void begin(StateProvider stateProvider,
             TelemetryProvider telemetryProvider,
             ConfigSaver configSaver,
             CommandHandler commandHandler);
  void loop();

 private:
  WebServer server_;
  StateProvider stateProvider_;
  TelemetryProvider telemetryProvider_;
  ConfigSaver configSaver_;
  CommandHandler commandHandler_;

  bool authenticated();
  void sendJson(int code, const String& body);
  void setupRoutes();
};
