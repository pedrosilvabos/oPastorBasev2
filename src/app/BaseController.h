#pragma once
#include <Arduino.h>
#include "net/loraManager/loraManager.h"
#include "net/messageQueue.h"
#include "net/pairingWindow.h"

class BaseController {
 public:
  static constexpr uint32_t PAIR_WINDOW_MS = 30000;  // 30 s
  static constexpr size_t MAX_MESSAGES = 32;

  bool begin();
  void loopOnce();

 private:
  void handleInbound_(const String& msg);
  void onPairingReq_(const String& msg);
  void drainQueue_();

 private:
  LoRaManager lora_;
  PairingWindow pairWin_{PAIR_WINDOW_MS};
  MessageQueue<MAX_MESSAGES> outbox_;
};
