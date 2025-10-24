#pragma once
#include <Arduino.h>
#include "net/LoRaManager.h"
#include "net/MessageQueue.h"
#include "net/PairingWindow.h"

class BaseController {
 public:
  static constexpr uint32_t PAIR_WINDOW_MS = 30000;  // 30 s
  static constexpr size_t MAX_MESSAGES = 32;

  bool begin() {
    Serial.begin(115200);
    delay(50);
    if (!lora_.begin()) {
      Serial.println("LoRa init failed");
      return false;
    }
    Serial.println("LoRa ready");
    return true;
  }

  void loopOnce() {
    // 1) Read any LoRa message
    String msg;
    if (lora_.receive(msg)) {
      handleInbound_(msg);
    }

    // 2) Transmit queued messages
    drainQueue_();
  }

 private:
  void handleInbound_(const String& msg) {
    // Expect formats like:
    // PAIRING_REQ,<nodeId>
    // DATA,<nodeId>,<payload...>
    if (msg.startsWith("PAIRING_REQ,")) {
      onPairingReq_(msg);
      return;
    }

    if (pairWin_.isOpen()) {
      // Accept other pairing follow-ups only inside window if needed.
      // Extend here if you define more steps.
    }

    // Handle other messages if required.
  }

  void onPairingReq_(const String& msg) {
    // Extract nodeId
    int comma = msg.indexOf(',');
    String nodeId = (comma >= 0) ? msg.substring(comma + 1) : "";

    // Open pairing window
    pairWin_.open();
    Serial.printf("PAIRING window open (%u ms) for node '%s'\n", PAIR_WINDOW_MS, nodeId.c_str());

    // Enqueue ACK
    String ack = String("PAIRING_ACK,") + nodeId + "," + String(millis());
    if (!outbox_.enqueue(ack)) {
      Serial.println("Outbox full. Dropping PAIRING_ACK");
    }
  }

  void drainQueue_() {
    // Only send while window is open if message is pairing-related.
    // Generic rule: always send. If you need throttling, add it.
    if (outbox_.isEmpty()) return;

    String next;
    if (!outbox_.dequeue(next)) return;

    bool ok = lora_.send(next);
    Serial.printf("TX: %s [%s]\n", next.c_str(), ok ? "ok" : "fail");
  }

 private:
  LoRaManager lora_;
  PairingWindow pairWin_{PAIR_WINDOW_MS};
  MessageQueue<MAX_MESSAGES> outbox_;
};
