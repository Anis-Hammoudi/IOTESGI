#include "LightSensor.h"

#include "AppConfig.h"

LightSensor::LightSensor(uint8_t pin) : pin_(pin) {}

void LightSensor::begin() {
  pinMode(pin_, INPUT);
  analogReadResolution(12);
  uint16_t initial = analogRead(pin_);
  Serial.printf("[light] HW-486 initial raw value: %u (threshold=%u)\n", initial, AppConfig::LightOnThreshold);
}

uint16_t LightSensor::readRaw() const {
  return analogRead(pin_);
}

bool LightSensor::isReactorOnline(uint16_t rawValue) const {
  return rawValue >= AppConfig::LightOnThreshold;
}

