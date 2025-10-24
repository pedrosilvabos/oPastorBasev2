#pragma once
#include <Arduino.h>

class PairingWindow {
 public:
  explicit PairingWindow(uint32_t duration_ms) : dur_(duration_ms) {}
  void open() {
    start_ = millis();
    open_ = true;
  }
  void close() {
    open_ = false;
  }
  bool isOpen() const {
    if (!open_) return false;
    if (millis() - start_ > dur_) return false;
    return true;
  }
  uint32_t remainingMs() const {
    if (!isOpen()) return 0;
    uint32_t elapsed = millis() - start_;
    return (elapsed >= dur_) ? 0 : dur_ - elapsed;
  }

 private:
  uint32_t dur_;
  uint32_t start_ = 0;
  bool open_ = false;
};
