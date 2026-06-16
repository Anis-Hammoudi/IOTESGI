#pragma once

#include <Arduino.h>

class LightSensor {
 public:
  explicit LightSensor(uint8_t pin);
  void begin();
  uint16_t readRaw() const;
  bool isReactorOnline(uint16_t rawValue) const;

 private:
  uint8_t pin_;
};
