#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <functional>

#include "AppConfig.h"
#include "TelemetryJson.h"
#include "actuators/AlarmBuzzer.h"
#include "actuators/ControlRodServo.h"
#include "actuators/StatusLed.h"
#include "config/RuntimeConfig.h"
#include "models/ReactorState.h"
#include "network/ConnectivityManager.h"
#include "network/MqttClientManager.h"
#include "sensors/EncoderInput.h"
#include "sensors/EnvironmentSensor.h"
#include "sensors/LightSensor.h"
#include "sensors/RadiationSensor.h"
#include "storage/OfflineStore.h"
#include "web/WebServerManager.h"

namespace {
SemaphoreHandle_t stateMutex;
ReactorState reactorState;

RuntimeConfig runtimeConfig;
ConnectivityManager connectivity;
MqttClientManager mqtt;
OfflineStore offlineStore;
WebServerManager webServer;

LightSensor lightSensor(AppConfig::PinLight);
RadiationSensor radiationSensor(AppConfig::PinOneWire);
EnvironmentSensor environmentSensor(AppConfig::PinDht);
EncoderInput encoder(AppConfig::PinEncoderClk, AppConfig::PinEncoderDt, AppConfig::PinEncoderSw);
StatusLed statusLed(AppConfig::PinLedRed, AppConfig::PinLedGreen, AppConfig::PinLedBlue);
AlarmBuzzer alarmBuzzer(AppConfig::PinBuzzer);
ControlRodServo controlRodServo(AppConfig::PinServo);
uint32_t lastDiagnosticMs = 0;

ReactorState copyState() {
  ReactorState copy;
  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    copy = reactorState;
    xSemaphoreGive(stateMutex);
  }
  return copy;
}

void mutateState(const std::function<void(ReactorState&)>& mutator) {
  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    mutator(reactorState);
    xSemaphoreGive(stateMutex);
  }
}

ReactorStatus evaluateStatus(const ReactorState& state) {
  if (state.emergencyStop) {
    return ReactorStatus::EmergencyStop;
  }
  if (!state.reactorOnline) {
    return ReactorStatus::Offline;
  }

  // Alarme critique : (humidité >= 60% ET temp room >= 40°C) OU radiation >= 32°C
  const bool radiationCritical = !isnan(state.coreTemperatureC) &&
                                  state.coreTemperatureC >= AppConfig::CoreCriticalThresholdC;
  const bool roomAndHumidityCritical =
      (!isnan(state.roomTemperatureC) && state.roomTemperatureC >= AppConfig::RoomCriticalThresholdC) &&
      (!isnan(state.roomHumidityPercent) && state.roomHumidityPercent >= AppConfig::HumidityCriticalThresholdPercent);

  if (radiationCritical || roomAndHumidityCritical) {
    return ReactorStatus::Critical;
  }

  // Avertissement précoce
  const bool coreWarning = !isnan(state.coreTemperatureC) && state.coreTemperatureC >= AppConfig::CoreWarningThresholdC;
  const bool roomWarning = !isnan(state.roomTemperatureC) && state.roomTemperatureC >= AppConfig::RoomWarningThresholdC;
  const bool humidityWarning = !isnan(state.roomHumidityPercent) &&
                               state.roomHumidityPercent >= AppConfig::HumidityWarningThresholdPercent;
  if (coreWarning || roomWarning || humidityWarning) {
    return ReactorStatus::Warning;
  }
  return ReactorStatus::Nominal;
}

void applyCommandPayload(const String& payload) {
  mutateState([&payload](ReactorState& state) {
    validateAndApplyCommand(payload, state);
  });
}

String printableFloat(float value, unsigned int digits = 1) {
  return isnan(value) ? String("NA") : String(value, digits);
}

String printableMeasurement(float value, const char* unit, unsigned int digits = 1) {
  if (isnan(value)) {
    return "NA";
  }
  return String(value, digits) + unit;
}

void printDiagnostic(const ReactorState& state) {
  Serial.println("--- [reactor] Diagnostic ---");
  Serial.printf("  Status       : %s\n", reactorStatusToString(state.status));
  Serial.printf("  Reactor      : %s\n", state.reactorOnline ? "ONLINE" : "OFFLINE");
  Serial.printf("  Light (raw)  : %u\n", state.lightRaw);
  Serial.printf("  Core temp    : %s\n", printableMeasurement(state.coreTemperatureC, "C").c_str());
  Serial.printf("  Room temp    : %s\n", printableMeasurement(state.roomTemperatureC, "C").c_str());
  Serial.printf("  Room humidity: %s\n", printableMeasurement(state.roomHumidityPercent, "%%").c_str());
  Serial.printf("  Rod position : %d%%\n", state.rodPositionPercent);
  Serial.printf("  WiFi         : %s\n", state.wifiConnected ? "OK" : "DOWN");
  Serial.printf("  MQTT         : %s\n", state.mqttConnected ? "OK" : "DOWN");
  Serial.printf("  IP           : %s\n", state.wifiConnected ? WiFi.localIP().toString().c_str() : "-");
  Serial.printf("  Free heap    : %u bytes\n", state.freeHeap);
  Serial.printf("  Offline queue: %u\n", static_cast<unsigned int>(state.offlineQueueSize));
  Serial.println("----------------------------");
}

void acquisitionTask(void*) {
  TickType_t lastWake = xTaskGetTickCount();
  bool firstCorePrinted = false;
  bool firstEnvPrinted = false;
  for (;;) {
    const uint16_t lightRaw = lightSensor.readRaw();
    const float coreTemperature = radiationSensor.readCoreTemperatureC();
    const EnvironmentReadings environment = environmentSensor.read();

    // Log la première lecture réussie de chaque capteur
    if (!firstCorePrinted && !isnan(coreTemperature)) {
      Serial.printf("[acq] First DS18B20 reading: %.1f C\n", coreTemperature);
      firstCorePrinted = true;
    }
    if (!firstEnvPrinted && environment.valid) {
      Serial.printf("[acq] First DHT22 reading: T=%.1f C  H=%.1f%%\n",
                    environment.temperatureC, environment.humidityPercent);
      firstEnvPrinted = true;
    }

    mutateState([&](ReactorState& state) {
      state.timestampMs = millis();
      state.lightRaw = lightRaw;
      state.reactorOnline = lightSensor.isReactorOnline(lightRaw);
      state.coreTemperatureC = coreTemperature;
      if (environment.valid) {
        state.roomTemperatureC = environment.temperatureC;
        state.roomHumidityPercent = environment.humidityPercent;
      }
    });

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::AcquisitionPeriodMs));
  }
}

void controlTask(void*) {
  TickType_t lastWake = xTaskGetTickCount();
  int lastEncoderPos = encoder.positionPercent();
  bool wasInAlarm = false; // Pour détecter la transition vers l'alarme

  for (;;) {
    encoder.update();
    const ButtonEvent buttonEvent = encoder.consumeButtonEvent();

    ReactorState snapshot = copyState();
    const int currentEncoderPos = encoder.positionPercent();

    // L'opérateur peut TOUJOURS modifier la position des barres
    if (currentEncoderPos != lastEncoderPos) {
      snapshot.rodPositionPercent = currentEncoderPos;
      lastEncoderPos = currentEncoderPos;
    } else if (snapshot.rodPositionPercent != lastEncoderPos) {
      encoder.setPositionPercent(snapshot.rodPositionPercent);
      lastEncoderPos = snapshot.rodPositionPercent;
    }

    // Bouton : appui court = acquitter alarme, appui long = arrêt urgence
    if (buttonEvent == ButtonEvent::ShortPress) {
      snapshot.alarmAcknowledged = true;
    }
    if (buttonEvent == ButtonEvent::LongPress) {
      snapshot.emergencyStop = true;
      snapshot.alarmAcknowledged = true; // Silence buzzer on emergency stop
    }

    snapshot.status = evaluateStatus(snapshot);

    // À l'entrée en alarme : barres automatiquement à 100%
    const bool inAlarm = (snapshot.status == ReactorStatus::Critical ||
                          snapshot.status == ReactorStatus::EmergencyStop);
    if (inAlarm && !wasInAlarm) {
      // Transition vers l'alarme : forcer les barres à 100%
      snapshot.rodPositionPercent = 100;
      encoder.setPositionPercent(100);
      lastEncoderPos = 100;
    }
    wasInAlarm = inAlarm;
    // Après la transition, l'opérateur peut librement modifier la position

    controlRodServo.setInsertionPercent(snapshot.rodPositionPercent);
    statusLed.show(snapshot.status, snapshot.mqttConnected);
    alarmBuzzer.setActive(inAlarm && !snapshot.alarmAcknowledged);

    mutateState([&snapshot](ReactorState& state) {
      state.alarmAcknowledged = snapshot.alarmAcknowledged;
      state.emergencyStop = snapshot.emergencyStop;
      state.rodPositionPercent = snapshot.rodPositionPercent;
      state.status = snapshot.status;
    });

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::ControlPeriodMs));
  }
}

void networkTask(void*) {
  TickType_t lastWake = xTaskGetTickCount();
  uint32_t lastPublishMs = 0;
  for (;;) {
    connectivity.loop();
    mqtt.loop(connectivity.wifiConnected());

    mutateState([](ReactorState& state) {
      state.wifiConnected = connectivity.wifiConnected();
      state.mqttConnected = mqtt.connected();
    });

    const uint32_t now = millis();
    if (now - lastPublishMs >= AppConfig::MqttPublishPeriodMs) {
      ReactorState snapshot = copyState();
      const String payload = buildTelemetryJson(snapshot);

      if (mqtt.connected()) {
        offlineStore.replay([](const String& storedPayload) {
          return mqtt.publishTelemetry(storedPayload);
        });
      }

      if (!mqtt.publishTelemetry(payload)) {
        offlineStore.append(payload);
      }

      mutateState([](ReactorState& state) {
        state.offlineQueueSize = offlineStore.count();
      });
      lastPublishMs = now;
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(100));
  }
}

void webTask(void*) {
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    webServer.loop();
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::WebPeriodMs));
  }
}

void supervisorTask(void*) {
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    mutateState([](ReactorState& state) {
      state.freeHeap = ESP.getFreeHeap();
      state.uptimeMs = millis();
      state.offlineQueueSize = offlineStore.count();
    });

    const uint32_t now = millis();
    if (now - lastDiagnosticMs >= 5000) {
      printDiagnostic(copyState());
      lastDiagnosticMs = now;
    }
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::SupervisorPeriodMs));
  }
}
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("[reactor] booting Nuclear Reactor supervision station");
  stateMutex = xSemaphoreCreateMutex();

  if (LittleFS.begin(true)) {
    Serial.println("[reactor] LittleFS mounted");
  } else {
    Serial.println("[reactor] LittleFS mount failed");
  }
  runtimeConfig.begin();
  offlineStore.begin();

  Serial.println("[reactor] Initialising sensors...");
  lightSensor.begin();
  Serial.println("[reactor] HW-486 (Light) OK");
  radiationSensor.begin();
  Serial.println("[reactor] DS18B20 (Core temp) init done");
  environmentSensor.begin();
  Serial.println("[reactor] DHT22 (Room env) init done");
  encoder.begin();
  Serial.println("[reactor] HW-040 (Encoder) OK");
  statusLed.begin();
  Serial.println("[reactor] LEDs OK");
  alarmBuzzer.begin();
  Serial.println("[reactor] HW-508 (Buzzer) OK");
  controlRodServo.begin();
  Serial.println("[reactor] SG90 (Servo) OK");

  connectivity.begin();
  mqtt.begin(runtimeConfig.mqtt());
  mqtt.setCommandHandler(applyCommandPayload);
  Serial.println("[reactor] WiFi/MQTT tasks configured");

  webServer.begin(
      copyState,
      []() { return buildTelemetryJson(copyState()); },
      [](const MqttSettings& settings) { return runtimeConfig.saveMqtt(settings); },
      applyCommandPayload);
  Serial.println("[reactor] Web server configured");

  xTaskCreatePinnedToCore(acquisitionTask, "acquisition", 4096, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(controlTask, "control", 4096, nullptr, 4, nullptr, 1);
  xTaskCreatePinnedToCore(networkTask, "network", 6144, nullptr, 2, nullptr, 0);
  xTaskCreatePinnedToCore(webTask, "web", 6144, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(supervisorTask, "supervisor", 3072, nullptr, 1, nullptr, 0);
}

void loop() {
  vTaskDelete(NULL);
}
