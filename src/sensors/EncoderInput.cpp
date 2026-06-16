#include "EncoderInput.h"

namespace {
constexpr uint32_t LongPressMs = 1500;
constexpr uint32_t DebounceMs = 5;
constexpr int Step = 5;
}

EncoderInput::EncoderInput(uint8_t clkPin, uint8_t dtPin, uint8_t swPin)
    : clkPin_(clkPin), dtPin_(dtPin), swPin_(swPin) {}

void EncoderInput::begin() {
  pinMode(clkPin_, INPUT_PULLUP);
  pinMode(dtPin_, INPUT_PULLUP);
  pinMode(swPin_, INPUT_PULLUP);
  lastClk_ = digitalRead(clkPin_);
  lastButton_ = digitalRead(swPin_);
}

void EncoderInput::update() {
  const uint32_t now = millis();

  // --- Lecture encodeur par polling (anti-rebond) ---
  const int clk = digitalRead(clkPin_);
  if (clk != lastClk_ && (now - lastRotationMs_ >= DebounceMs)) {
    const int dt = digitalRead(dtPin_);
    if (dt != clk) {
      positionPercent_ += Step;
    } else {
      positionPercent_ -= Step;
    }
    if (positionPercent_ > 100) positionPercent_ = 100;
    if (positionPercent_ < 0) positionPercent_ = 0;
    lastRotationMs_ = now;
  }
  lastClk_ = clk;

  // --- Lecture bouton ---
  const int button = digitalRead(swPin_);
  if (lastButton_ == HIGH && button == LOW) {
    buttonPressedAtMs_ = now;
  }
  if (lastButton_ == LOW && button == HIGH) {
    pendingButtonEvent_ = (now - buttonPressedAtMs_ >= LongPressMs)
                            ? ButtonEvent::LongPress
                            : ButtonEvent::ShortPress;
  }
  lastButton_ = button;
}

int EncoderInput::positionPercent() const {
  return positionPercent_;
}

void EncoderInput::setPositionPercent(int value) {
  positionPercent_ = constrain(value, 0, 100);
}

ButtonEvent EncoderInput::consumeButtonEvent() {
  const ButtonEvent event = pendingButtonEvent_;
  pendingButtonEvent_ = ButtonEvent::None;
  return event;
}
