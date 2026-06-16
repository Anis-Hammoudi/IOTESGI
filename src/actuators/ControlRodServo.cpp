#include "ControlRodServo.h"

ControlRodServo::ControlRodServo(uint8_t pin) : pin_(pin) {}

void ControlRodServo::begin() {
  servo_.setPeriodHertz(50);
  servo_.attach(pin_, 500, 2400);
  setInsertionPercent(100);
}

void ControlRodServo::setInsertionPercent(int percent) {
  const int constrainedPercent = constrain(percent, 0, 100);
  const int angle = map(constrainedPercent, 0, 100, 0, 180);
  servo_.write(angle);
}
