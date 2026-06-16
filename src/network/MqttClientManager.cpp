#include "MqttClientManager.h"

namespace {
constexpr uint32_t MqttRetryMs = 5000;
constexpr uint8_t TelemetryQos = 1;
MqttClientManager* activeManager = nullptr;

void handleMqttMessage(int messageSize) {
  if (activeManager != nullptr) {
    activeManager->handleMessage(messageSize);
  }
}
}

void MqttClientManager::begin(const MqttSettings& settings) {
  activeManager = this;
  settings_ = settings;
  const String clientId = settings_.baseTopic + "/client";
  mqttClient_.setId(clientId.c_str());
  if (settings_.username.length() > 0) {
    mqttClient_.setUsernamePassword(settings_.username, settings_.password);
  }
  mqttClient_.onMessage(handleMqttMessage);
}

void MqttClientManager::loop(bool wifiConnected) {
  if (!wifiConnected) {
    return;
  }

  if (!mqttClient_.connected()) {
    const uint32_t now = millis();
    if (now - lastAttemptMs_ >= MqttRetryMs) {
      connect();
      lastAttemptMs_ = now;
    }
    return;
  }

  mqttClient_.poll();
}

bool MqttClientManager::connected() {
  return mqttClient_.connected();
}

bool MqttClientManager::publishTelemetry(const String& payload) {
  if (!connected()) {
    return false;
  }

  const String topic = telemetryTopic();
  mqttClient_.beginMessage(topic.c_str(), false, TelemetryQos);
  mqttClient_.print(payload);
  return mqttClient_.endMessage() == 1;
}

void MqttClientManager::setCommandHandler(const std::function<void(const String&)>& handler) {
  commandHandler_ = handler;
}

String MqttClientManager::telemetryTopic() const {
  return settings_.baseTopic + "/data";
}

String MqttClientManager::commandTopic() const {
  return settings_.baseTopic + "/cmd";
}

void MqttClientManager::connect() {
  if (!mqttClient_.connect(settings_.host.c_str(), settings_.port)) {
    return;
  }
  const String topic = commandTopic();
  mqttClient_.subscribe(topic.c_str(), 1);
}

void MqttClientManager::handleMessage(int messageSize) {
  String payload;
  payload.reserve(messageSize);
  while (mqttClient_.available()) {
    payload += static_cast<char>(mqttClient_.read());
  }

  if (commandHandler_) {
    commandHandler_(payload);
  }
}
