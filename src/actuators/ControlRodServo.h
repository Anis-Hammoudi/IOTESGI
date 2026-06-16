#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>

class ControlRodServo {
 public:
  explicit ControlRodServo(uint8_t pin);
  void begin();
  void setInsertionPercent(int percent);

 private:
  Servo servo_;
  uint8_t pin_;
};
