#pragma once

#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <WiFi.h>
#include <functional>

#include "config/RuntimeConfig.h"

class MqttClientManager {
  friend void handleMqttMessage(int messageSize);

 public:
  void begin(const MqttSettings& settings);
  void loop(bool wifiConnected);
  bool connected();
  bool publishTelemetry(const String& payload);
  void setCommandHandler(const std::function<void(const String&)>& handler);
  String telemetryTopic() const;
  String commandTopic() const;
  void handleMessage(int messageSize);

 private:
  WiFiClient wifiClient_;
  MqttClient mqttClient_{wifiClient_};
  MqttSettings settings_;
  uint32_t lastAttemptMs_ = 0;
  std::function<void(const String&)> commandHandler_;

  void connect();
};
