#include "RuntimeConfig.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "AppConfig.h"

namespace {
constexpr const char* ConfigPath = "/config.json";
}

bool RuntimeConfig::begin() {
  if (!loadMqtt()) {
    mqttSettings_.baseTopic = String("campus/") + AppConfig::GroupId + "/" + AppConfig::DeviceId;
    return saveMqtt(mqttSettings_);
  }
  return true;
}

MqttSettings RuntimeConfig::mqtt() const {
  return mqttSettings_;
}

bool RuntimeConfig::saveMqtt(const MqttSettings& settings) {
  DynamicJsonDocument doc(512);
  doc["mqtt"]["host"] = settings.host;
  doc["mqtt"]["port"] = settings.port;
  doc["mqtt"]["username"] = settings.username;
  doc["mqtt"]["password"] = settings.password;
  doc["mqtt"]["baseTopic"] = settings.baseTopic.length() > 0
                                 ? settings.baseTopic
                                 : String("campus/") + AppConfig::GroupId + "/" + AppConfig::DeviceId;

  File file = LittleFS.open(ConfigPath, FILE_WRITE);
  if (!file) {
    return false;
  }

  const bool written = serializeJsonPretty(doc, file) > 0;
  file.close();

  if (written) {
    mqttSettings_ = settings;
    if (mqttSettings_.baseTopic.length() == 0) {
      mqttSettings_.baseTopic = String("campus/") + AppConfig::GroupId + "/" + AppConfig::DeviceId;
    }
  }

  return written;
}

bool RuntimeConfig::loadMqtt() {
  if (!LittleFS.exists(ConfigPath)) {
    return false;
  }

  File file = LittleFS.open(ConfigPath, FILE_READ);
  if (!file) {
    return false;
  }

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    return false;
  }

  JsonObject mqtt = doc["mqtt"].as<JsonObject>();
  mqttSettings_.host = mqtt["host"] | "broker.hivemq.com";
  mqttSettings_.port = mqtt["port"] | 1883;
  mqttSettings_.username = mqtt["username"] | "";
  mqttSettings_.password = mqtt["password"] | "";
  mqttSettings_.baseTopic = mqtt["baseTopic"] | "";

  if (mqttSettings_.baseTopic.length() == 0) {
    mqttSettings_.baseTopic = String("campus/") + AppConfig::GroupId + "/" + AppConfig::DeviceId;
  }

  return mqttSettings_.host.length() > 0;
}
