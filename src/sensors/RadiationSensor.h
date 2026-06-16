#pragma once

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>

class RadiationSensor {
 public:
  explicit RadiationSensor(uint8_t pin);
  void begin();
  float readCoreTemperatureC();

 private:
  uint8_t pin_;
  OneWire oneWire_;
  DallasTemperature dallas_;
  float filteredTemp_ = NAN;
  static constexpr float alpha_ = 0.2f;
};
