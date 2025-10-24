#pragma once
#include <Arduino.h>

template <size_t CAPACITY>
class MessageQueue {
 public:
  bool enqueue(const String& msg) {
    if (count_ >= CAPACITY) return false;
    buf_[tail_] = msg;
    tail_ = (tail_ + 1) % CAPACITY;
    ++count_;
    return true;
  }

  bool dequeue(String& out) {
    if (!count_) return false;
    out = buf_[head_];
    head_ = (head_ + 1) % CAPACITY;
    --count_;
    return true;
  }

  bool isEmpty() const {
    return count_ == 0;
  }
  size_t size() const {
    return count_;
  }
  size_t capacity() const {
    return CAPACITY;
  }

 private:
  String buf_[CAPACITY];
  size_t head_ = 0;
  size_t tail_ = 0;
  size_t count_ = 0;
};
