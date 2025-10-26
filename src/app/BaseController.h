#pragma once
#include <Arduino.h>
#include <Preferences.h>

#include "model/telemetry.h"
#include "net/MessageQueue.h"
#include "net/PairingWindow.h"
#include "net/loraManager/loraManager.h"

class LteConnectionManager;

class BaseController {
 public:
  // --- Constants ---
  static constexpr uint32_t PAIR_WINDOW_MS = 30000;
  static constexpr size_t MAX_MESSAGES = 32;

  // SYNC framing
  static constexpr uint32_t SYNC_INTERVAL_MS = 60000;   // 1 min between SYNC cycles
  static constexpr uint32_t WINDOW_DURATION_MS = 4000;  // 4 s RX window
  static constexpr uint32_t POST_GRACE_MS = 1500;       // RX drain
  static constexpr uint16_t SLOTS_PER_WINDOW = 30;      // Number of cows per window
  static constexpr uint16_t SLOT_DURATION_MS = 750;     // Wait for cow responde after SYNC:

  // Sleep helpers
  bool readyToSleep() const;             // no active window, nothing to send/post
  uint32_t timeUntilNextSyncMs() const;  // remaining ms to next SYNC

  // --- Lifecycle ---
  bool begin();
  void loopOnce();

  // --- LoRa I/O ---
  bool loraReceive(String& msg);

  // --- Telemetry batch (window -> cloud) ---
  bool hasBatchReady() const;
  bool popNextTelemetry(String& out);         // mainly for tests/diagnostics
  void attachLte(LteConnectionManager* lte);  // inject LTE dependency
  void postBatchToCloud();                    // post buffered lines using LTE

 private:
  // --- Inbound handlers ---
  void handleInbound_(const String& msg);
  void onPairingReq_(const String& msg);

  // --- Outbound queue ---
  void drainQueue_();

  // --- Provisioning ---
  void saveProvisionedCow_(const String& cowId, const String& mac);
  int getTotalProvisionedCows_();
  bool isProvisioned_(const String& mac, String* outCowId);
  String normCowId_(String id);

  // --- SYNC scheduler ---
  void tryStartSyncCycle_();
  void startWindow_(uint16_t windowIndex);
  void tickWindow_();
  String buildSyncMsg_(uint32_t t0, uint16_t windowIndex, uint16_t totalCows);

  // --- Parsing ---
  bool parseNodeCsv_(const String& line, Telemetry& out) const;

 private:
  // Devices
  LoRaManager lora_;
  PairingWindow pairWin_{PAIR_WINDOW_MS};
  Preferences prefCows_;                 // NVS namespace: "provisioning"
  LteConnectionManager* lte_ = nullptr;  // injected

  // Queues
  MessageQueue<MAX_MESSAGES> outbox_;
  MessageQueue<MAX_MESSAGES> telemBuf_;  // inbound telemetry

  // SYNC state
  uint32_t lastSyncMs_ = 0;
  uint32_t windowEndMs_ = 0;
  uint16_t totalCows_ = 0;
  uint16_t totalWindows_ = 0;
  uint16_t currWindow_ = 0;
  bool inCycle_ = false;
  bool cycleComplete_ = false;
};
