#pragma once

#include <Arduino.h>
#include <DHT.h>

struct EnvironmentReadings {
  float temperatureC = NAN;
  float humidityPercent = NAN;
  bool valid = false;
};

class EnvironmentSensor {
 public:
  explicit EnvironmentSensor(uint8_t pin);
  bool begin();
  EnvironmentReadings read();

 private:
  DHT dht_;
  bool available_ = false;
  float filteredTemp_ = NAN;
  float filteredHum_ = NAN;
  uint32_t readFailCount_ = 0;
  static constexpr float alpha_ = 0.2f;
};
