#pragma once

#include <Arduino.h>

namespace AppConfig {
constexpr const char *DeviceId = "reactor-esp32-01";
constexpr const char *GroupId = "groupe-nuclear";
constexpr const char *TelemetrySchema = "nuclear-reactor.telemetry.v1";

constexpr const char *WifiSsid = "Anis";
constexpr const char *WifiPassword = "anis2002";

constexpr uint8_t PinLight = 34;
constexpr uint8_t PinOneWire = 4;
constexpr uint8_t PinDht = 21;
constexpr uint8_t PinEncoderClk = 18;
constexpr uint8_t PinEncoderDt = 19;
constexpr uint8_t PinEncoderSw = 5;
constexpr uint8_t PinServo = 13;
constexpr uint8_t PinLedRed = 25;
constexpr uint8_t PinLedGreen = 26;
constexpr uint8_t PinLedBlue = 27;
constexpr uint8_t PinBuzzer = 14;

constexpr uint16_t LightOnThreshold = 1800;
constexpr float CoreWarningThresholdC = 45.0F;
constexpr float CoreCriticalThresholdC = 60.0F;
constexpr float RoomWarningThresholdC = 35.0F;
constexpr float RoomCriticalThresholdC = 45.0F;
constexpr float HumidityWarningThresholdPercent = 70.0F;
constexpr float HumidityCriticalThresholdPercent = 85.0F;

constexpr uint32_t AcquisitionPeriodMs = 2000;
constexpr uint32_t ControlPeriodMs = 50;
constexpr uint32_t MqttPublishPeriodMs = 10000;
constexpr uint32_t WebPeriodMs = 10;
constexpr uint32_t SupervisorPeriodMs = 1000;
} // namespace AppConfig
