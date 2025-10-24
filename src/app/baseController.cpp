#include "app/baseController.h"

bool BaseController::begin() {
  Serial.begin(115200);
  delay(50);
  if (!lora_.begin()) {
    Serial.println("LoRa init failed");
    return false;
  }
  Serial.println("LoRa ready");
  return true;
}

void BaseController::loopOnce() {
  String msg;
  if (lora_.receive(msg)) {
    handleInbound_(msg);
  }
  drainQueue_();
}

void BaseController::handleInbound_(const String& msg) {
  if (msg.startsWith("PAIRING_REQ,")) {
    onPairingReq_(msg);
    return;
  }

  if (pairWin_.isOpen()) {
    // Extend with other pairing follow-ups if needed.
  }

  // Add other message handlers here as required.
}

void BaseController::onPairingReq_(const String& msg) {
  const int comma = msg.indexOf(',');
  const String nodeId = (comma >= 0) ? msg.substring(comma + 1) : "";

  pairWin_.open();
  Serial.printf("PAIRING window open (%u ms) for node '%s'\n", PAIR_WINDOW_MS, nodeId.c_str());

  const String ack = String("PAIRING_ACK,") + nodeId + "," + String(millis());
  if (!outbox_.enqueue(ack)) {
    Serial.println("Outbox full. Dropping PAIRING_ACK");
  }
}

void BaseController::drainQueue_() {
  if (outbox_.isEmpty()) return;

  String next;
  if (!outbox_.dequeue(next)) return;

  const bool ok = lora_.send(next);
  Serial.printf("TX: %s [%s]\n", next.c_str(), ok ? "ok" : "fail");
}
