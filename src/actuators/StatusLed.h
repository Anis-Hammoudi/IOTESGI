#pragma once

#include <Arduino.h>
#include "models/ReactorState.h"

class StatusLed {
 public:
  StatusLed(uint8_t redPin, uint8_t greenPin, uint8_t bluePin);
  void begin();
  void show(ReactorStatus status, bool mqttConnected);

 private:
  void write(bool red, bool green, bool blue);
  uint8_t redPin_;
  uint8_t greenPin_;
  uint8_t bluePin_;
};
