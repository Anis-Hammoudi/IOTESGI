#include "EncoderInput.h"

namespace {
constexpr uint32_t LongPressMs = 1500;
constexpr int Sensitivity = 5;

// Global pointer for ISR (assuming only one encoder is used in the project)
EncoderInput* g_encoder = nullptr;

void IRAM_ATTR encoderISR() {
  if (g_encoder) {
    g_encoder->handleInterrupt();
  }
}
} // namespace

EncoderInput::EncoderInput(uint8_t clkPin, uint8_t dtPin, uint8_t swPin)
    : clkPin_(clkPin), dtPin_(dtPin), swPin_(swPin) {}

void EncoderInput::begin() {
  pinMode(clkPin_, INPUT_PULLUP);
  pinMode(dtPin_, INPUT_PULLUP);
  pinMode(swPin_, INPUT_PULLUP);
  lastButton_ = digitalRead(swPin_);
  
  g_encoder = this;
  attachInterrupt(digitalPinToInterrupt(clkPin_), encoderISR, FALLING);
}

void IRAM_ATTR EncoderInput::handleInterrupt() {
  // Read dtPin to determine direction
  if (digitalRead(dtPin_) == HIGH) {
    positionPercent_ += Sensitivity;
  } else {
    positionPercent_ -= Sensitivity;
  }
  
  // Constrain manually since constrain() might not be IRAM-safe on all cores
  if (positionPercent_ > 100) positionPercent_ = 100;
  if (positionPercent_ < 0) positionPercent_ = 0;
}

void EncoderInput::update() {
  // Encoder value is now updated by interrupts, we only need to poll the button
  const int button = digitalRead(swPin_);
  const uint32_t now = millis();
  
  if (lastButton_ == HIGH && button == LOW) {
    buttonPressedAtMs_ = now;
  }
  if (lastButton_ == LOW && button == HIGH) {
    pendingButtonEvent_ = now - buttonPressedAtMs_ >= LongPressMs
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
