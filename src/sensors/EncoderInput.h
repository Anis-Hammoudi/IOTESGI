#pragma once

#include <Arduino.h>

enum class ButtonEvent : uint8_t {
  None,
  ShortPress,
  LongPress
};

class EncoderInput {
 public:
  EncoderInput(uint8_t clkPin, uint8_t dtPin, uint8_t swPin);
  void begin();
  void update();
  int positionPercent() const;
  void setPositionPercent(int value);
  ButtonEvent consumeButtonEvent();
  void handleInterrupt();

 private:
  uint8_t clkPin_;
  uint8_t dtPin_;
  uint8_t swPin_;
  volatile int positionPercent_ = 0;

  int lastButton_ = HIGH;
  uint32_t buttonPressedAtMs_ = 0;
  ButtonEvent pendingButtonEvent_ = ButtonEvent::None;
};
