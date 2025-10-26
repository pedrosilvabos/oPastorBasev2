#include "app/BaseController.h"
#include "net/lteManager/lteConnectionManager.h"
#include "model/telemetry.h"

static void split2_(const String& s, char sep, String& a, String& b) {
  int i = s.indexOf(sep);
  if (i < 0) {
    a = s;
    b = "";
    return;
  }
  a = s.substring(0, i);
  b = s.substring(i + 1);
}

bool BaseController::readyToSleep() const {
  // not inside SYNC, no pending radio TX, no batch to post
  return !inCycle_ && outbox_.isEmpty() && !hasBatchReady();
}

uint32_t BaseController::timeUntilNextSyncMs() const {
  if (lastSyncMs_ == 0) return 0;  // first boot: start immediately
  const uint32_t now = millis();
  const uint32_t next = lastSyncMs_ + SYNC_INTERVAL_MS;
  if (next <= now) return 0;
  return next - now;
}

bool BaseController::parseNodeCsv_(const String& line, Telemetry& out) const {
  // Expect 12 commas → 13 fields:
  // cow, lat, lon, alert, nBatt, nBattPct, nVBUS, nHasBatt, sat, fix, course, alt, speed
  int c[12], idx = -1;
  for (int i = 0; i < 12; ++i) {
    int p = line.indexOf(',', idx + 1);
    if (p < 0) return false;
    c[i] = p;
    idx = p;
  }
  auto sub = [&](int a, int b) { return line.substring(a, b); };

  String cowRaw = sub(0, c[0]);
  cowRaw.trim();
  String sLat = sub(c[0] + 1, c[1]);
  String sLon = sub(c[1] + 1, c[2]);
  String sAlert = sub(c[2] + 1, c[3]);
  String sNBatt = sub(c[3] + 1, c[4]);
  String sNBattPc = sub(c[4] + 1, c[5]);
  String sNVBUS = sub(c[5] + 1, c[6]);
  String sNHasBat = sub(c[6] + 1, c[7]);
  String sSat = sub(c[7] + 1, c[8]);          // unused
  String sFix = sub(c[8] + 1, c[9]);          // unused
  String sCourse = sub(c[9] + 1, c[10]);      // unused
  String sAlt = sub(c[10] + 1, c[11]);        // unused
  String sSpeed = line.substring(c[11] + 1);  // unused

  String cowIdStr = String("ESPCOW_") + cowRaw;
  out.cowId = strdup(cowIdStr.c_str());
  out.baseId = "base_001";
  out.name = "";
  out.tagId = "";
  out.birthDate = "";
  out.breed = "";

  out.latitude = sLat.toFloat();
  out.longitude = sLon.toFloat();
  out.nodeTemperature = 0.0f;

  out.nodeBattery = sNBatt.toFloat();
  out.nodeBatteryPercent = sNBattPc.toInt();

  out.baseBattery = 0.0f;
  out.baseBatteryPercent = 0;

  out.isAlerted = (sAlert.toInt() != 0);
  out.alertType = 0;

  out.nodeVbus = sNVBUS.toFloat();

  out.nodeHasBattery = (sNHasBat.toInt() != 0) ? 1 : 0;

  return true;
}

void BaseController::attachLte(LteConnectionManager* lte) {
  lte_ = lte;
}

void BaseController::postBatchToCloud() {
  if (!lte_) {
    Serial.println("LTE not attached");
    return;
  }
  if (!hasBatchReady()) return;

  String line;
  int posted = 0;
  while (popNextTelemetry(line)) {
    Telemetry t{};
    if (!parseNodeCsv_(line, t)) {
      Serial.printf("⚠️ Malformed telemetry dropped: %s\n", line.c_str());
      continue;
    }
    if (lte_->postTelemetry(t))
      ++posted;
    else {
      Serial.println("❌ postTelemetry failed; stopping batch");
      break;
    }
  }
  cycleComplete_ = false;
  Serial.printf("✅ Posted %d telemetry item(s)\n", posted);
}

bool BaseController::begin() {
  Serial.begin(115200);
  delay(50);
  if (!lora_.begin()) {
    Serial.println("LoRa init failed");
    return false;
  }
  if (!prefCows_.begin("provisioning", false)) {
    Serial.println("NVS open failed");
    return false;
  }
  Serial.println("LoRa ready");
  return true;
}

void BaseController::loopOnce() {
  // 1) RX
  String msg;
  if (lora_.receive(msg)) {
    handleInbound_(msg);
  }

  // 2) SYNC scheduler
  if (!inCycle_) {
    tryStartSyncCycle_();
  } else {
    tickWindow_();
  }

  // 3) TX
  drainQueue_();
}

bool BaseController::loraReceive(String& msg) {
  return lora_.receive(msg);
}

// ---------- inbound ----------
void BaseController::handleInbound_(const String& msg) {
  if (msg.startsWith("PAIRING_REQ,")) {
    onPairingReq_(msg);
    return;
  }

  if (msg.startsWith("cow_")) {
    Serial.printf("📥 TELEM: %s\n", msg.c_str());
    if (!telemBuf_.enqueue(msg)) Serial.println("telemetry buffer full, drop");
    return;
  }
}

String BaseController::normCowId_(String id) {
  id.trim();
  if (!id.startsWith("cow_")) id = String("cow_") + id;
  return id;
}

// idempotent provisioning
bool BaseController::isProvisioned_(const String& mac, String* outCowId) {
  String id = prefCows_.getString(mac.c_str(), "");
  if (id.length()) id = normCowId_(id);  // normalize legacy values
  if (outCowId) *outCowId = id;
  return id.length() > 0;
}

void BaseController::onPairingReq_(const String& msg) {
  // form: PAIRING_REQ,<mac>
  int comma = msg.indexOf(',');
  if (comma < 0) return;

  String mac = msg.substring(comma + 1);
  mac.trim();

  String cowId = prefCows_.getString(mac.c_str(), "");

  if (cowId.isEmpty()) {
    int count = prefCows_.getInt("cow_count", 0);
    cowId = "cow_" + String(count);
    saveProvisionedCow_(cowId, mac);
    Serial.printf("✅ Provisioned %s as %s\n", mac.c_str(), cowId.c_str());
  } else {
    if (!cowId.startsWith("cow_")) cowId = "cow_" + cowId;
    Serial.printf("ℹ️ Already paired: %s -> %s\n", mac.c_str(), cowId.c_str());
  }

  pairWin_.open();
  Serial.printf("PAIRING window open (%u ms) cowId='%s' mac='%s'\n", PAIR_WINDOW_MS, cowId.c_str(),
                mac.c_str());

  // include MAC so only the matching node accepts the ACK
  String ack = "PROVISION_ACK," + cowId + "," + mac;

  for (int i = 0; i < 2; ++i) {
    bool ok = lora_.send(ack);  // must force TX internally
    Serial.printf("TX ACK[%d]: %s [%s]\n", i, ack.c_str(), ok ? "ok" : "fail");
    delay(30);  // small gap
  }

  (void)outbox_.enqueue(ack);
}

void BaseController::saveProvisionedCow_(const String& cowIdIn, const String& mac) {
  const String cowId = normCowId_(cowIdIn);  // enforce prefix on write
  prefCows_.putString(mac.c_str(), cowId);

  int count = prefCows_.getInt("cow_count", 0);
  prefCows_.putString((String("cow_") + count).c_str(), cowId);
  prefCows_.putString((String("mac_") + count).c_str(), mac);
  prefCows_.putInt("cow_count", count + 1);
}

int BaseController::getTotalProvisionedCows_() {
  const int count = prefCows_.getInt("cow_count", 0);
  // Optional: enumerate for diagnostics
  Serial.println("[PROVISIONING] Stored cows:");
  for (int i = 0; i < count; i++) {
    String key = "cow_" + String(i);
    String cowId = prefCows_.getString(key.c_str(), "");
    Serial.printf("  #%d: %s\n", i, cowId.c_str());
  }
  return count;
}
bool BaseController::hasBatchReady() const {
  return cycleComplete_ && !telemBuf_.isEmpty();
}

bool BaseController::popNextTelemetry(String& out) {
  return telemBuf_.dequeue(out);
}

// ---------- SYNC scheduler ----------
void BaseController::tryStartSyncCycle_() {
  const uint32_t now = millis();
  if (lastSyncMs_ != 0 && now - lastSyncMs_ < SYNC_INTERVAL_MS) {
    return;  // not yet time
  }

  totalCows_ = getTotalProvisionedCows_();
  Serial.printf("🐄 Total provisioned cows: %d\n", totalCows_);

  if (totalCows_ <= 0) {
    // No cows provisioned: still update lastSync to avoid hammering.
    lastSyncMs_ = now;
    return;
  }
  cycleComplete_ = false;
  totalWindows_ = (totalCows_ + SLOTS_PER_WINDOW - 1) / SLOTS_PER_WINDOW;
  currWindow_ = 0;
  inCycle_ = true;

  Serial.printf("🚀 Starting SYNC cycle with %u window(s)\n", totalWindows_);
  startWindow_(currWindow_);
}

void BaseController::startWindow_(uint16_t windowIndex) {
  const uint32_t t0 = millis();
  const String sync = buildSyncMsg_(t0, windowIndex, totalCows_);  // not 1
  (void)outbox_.enqueue(sync);
  (void)outbox_.enqueue(sync);  // redundancy
  windowEndMs_ = t0 + WINDOW_DURATION_MS;
  Serial.printf("📡 Broadcasting %s\n", sync.c_str());
}

static String normCowId_(String id) {
  id.trim();
  if (!id.startsWith("cow_")) id = String("cow_") + id;
  return id;
}

String BaseController::buildSyncMsg_(uint32_t t0, uint16_t windowIndex, uint16_t totalCows) {
  // "SYNC:<t0Sec>|<slotSec>|<windowSec>|<startSlot>|<totalCows>"
  const uint32_t t0Sec = t0 / 1000UL;
  const uint16_t slotSec = (SLOT_DURATION_MS + 999UL) / 1000UL;      // ceil to >=1
  const uint16_t windowSec = (WINDOW_DURATION_MS + 999UL) / 1000UL;  // ceil
  const uint16_t startSlot = windowIndex * SLOTS_PER_WINDOW;

  String s = "SYNC:";
  s += String(t0Sec) + "|";
  s += String(slotSec) + "|";
  s += String(windowSec) + "|";
  s += String(startSlot) + "|";
  s += String(totalCows);
  return s;
}

void BaseController::tickWindow_() {
  const uint32_t now = millis();

  // Stay in RX until end + grace
  if (now < windowEndMs_ + POST_GRACE_MS) {
    return;
  }

  // Move to next window or close cycle
  currWindow_++;
  if (currWindow_ < totalWindows_) {
    Serial.printf("➡️  Next SYNC window [%u/%u]\n", currWindow_ + 1, totalWindows_);
    startWindow_(currWindow_);
  } else {
    inCycle_ = false;
    lastSyncMs_ = now;
    cycleComplete_ = true;  // <-- signal batch ready
    Serial.printf("🌀 Completed full SYNC cycle\n");
  }
}

// ---------- TX drain ----------
void BaseController::drainQueue_() {
  String next;
  while (outbox_.dequeue(next)) {
    const bool ok = lora_.send(next);
    Serial.printf("TX: %s [%s]\n", next.c_str(), ok ? "ok" : "fail");
    delay(8);  // TX→RX turnaround
  }
}
