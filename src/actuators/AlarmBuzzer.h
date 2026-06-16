#pragma once

#include <Arduino.h>

class AlarmBuzzer {
 public:
  explicit AlarmBuzzer(uint8_t pin);
  void begin();
  void setActive(bool active);

 private:
  uint8_t pin_;
};
