#pragma once

#include <Arduino.h>

struct MqttSettings {
  String host = "broker.hivemq.com";
  uint16_t port = 1883;
  String username = "";
  String password = "";
  String baseTopic = "";
};

class RuntimeConfig {
 public:
  bool begin();
  MqttSettings mqtt() const;
  bool saveMqtt(const MqttSettings& settings);

 private:
  MqttSettings mqttSettings_;
  bool loadMqtt();
};
