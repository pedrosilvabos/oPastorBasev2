#include "net/lteManager/lteConnectionManager.h"
#include <ESP_SSLClient.h>
#include <ArduinoJson.h>

LteConnectionManager::LteConnectionManager()
    : serial_(2), modem_(serial_), netClient_(modem_), ssl_(nullptr), isConnected_(false) {}

LteConnectionManager::~LteConnectionManager() {
  delete ssl_;
}

bool LteConnectionManager::sendSms(const String& number, const String& text) {
  // Radio must be on and registered. PPP not required.
  if (!ensureConnected()) return false;
  bool ok = modem_.sendSMS(number, text);

  return ok;
}

void LteConnectionManager::begin() {
  Serial.begin(115200);
  delay(100);
  setupPins_();
  setFlightMode_(false);
  powerOnModem_();

  serial_.begin(UART_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);

  Serial.print("AT probe...");
  if (!modem_.testAT(10000)) {
    Serial.println("fail. Power-cycling once.");
    powerOnModem_();
    if (!modem_.testAT(10000)) {
      Serial.println("FATAL: no AT response.");
      return;
    }
  }
  Serial.println("ok");

  if (!ssl_) ssl_ = new ESP_SSLClient();
  ssl_->setInsecure();
  ssl_->setBufferSizes(SSL_RX_BUF, SSL_TX_BUF);
  ssl_->setSessionTimeout(SSL_SESSION_SEC);
  ssl_->setClient(&netClient_);

  Serial.println("=== init done ===");
}

bool LteConnectionManager::ensureConnected() {
  if (!isConnected_) {
    Serial.println("Modem off — powering on...");
    powerOnModem_();
  }

  if (modem_.isGprsConnected()) return true;

  Serial.println("Reconnecting data...");
  modem_.gprsDisconnect();

  if (!connectSimAndNetwork_()) return false;
  if (!startPdp_()) return false;

  return true;
}

bool LteConnectionManager::postTelemetry(const Telemetry& t) {
  if (!modem_.isGprsConnected()) {
    Serial.println("ERROR: GPRS not connected");
    return false;
  }

  String body = buildTelemetryJson_(t);
  Serial.printf("Payload size: %d\n", body.length());
  Serial.printf("TLS connect %s:%d\n", API_SERVER, API_PORT);

  if (!ssl_->connect(API_SERVER, API_PORT)) {
    Serial.println("FATAL: TLS connect failed");
    return false;
  }

  ssl_->print(F("POST "));
  ssl_->print(POST_TELEMETRY_ENDPOINT);
  ssl_->println(F(" HTTP/1.1"));
  ssl_->print(F("Host: "));
  ssl_->println(API_SERVER);
  ssl_->println(F("Content-Type: application/json"));
  ssl_->print(F("Content-Length: "));
  ssl_->println(body.length());
  ssl_->println(F("Connection: close"));
  ssl_->println("");
  ssl_->print(body);

  Serial.print("Reading response...");
  const uint32_t start = millis();
  while (!ssl_->available() && millis() - start < HTTP_RESP_TIMEOUT) delay(10);
  Serial.println();

  if (ssl_->available()) {
    Serial.println("--- HTTP Response ---");
    while (ssl_->available()) Serial.write(ssl_->read());
    Serial.println();
  } else {
    Serial.println("No response within timeout");
  }

  ssl_->stop();
  Serial.println("--- POST done. TLS closed. ---");
  return true;
}

void LteConnectionManager::disconnect() {
  modem_.gprsDisconnect();
}

// ---------- private ----------
void LteConnectionManager::setupPins_() {
  pinMode(MODEM_PWRKEY, OUTPUT);
  pinMode(MODEM_DTR, OUTPUT);
  pinMode(MODEM_FLIGHT, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(MODEM_PWRKEY, LOW);
  digitalWrite(MODEM_DTR, HIGH);
  digitalWrite(LED_PIN, LOW);
}

void LteConnectionManager::powerOnModem_() {
  Serial.println("Modem power sequence");
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(100);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(5000);
  isConnected_ = true;
}

void LteConnectionManager::shutdown() {
  powerOffModem_();
  isConnected_ = false;
}

void LteConnectionManager::powerOffModem_() {
  Serial.println("Modem power-off sequence");
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(2500);  // keep high >2 s to request shutdown
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(5000);  // allow full power-down
  isConnected_ = false;
}

void LteConnectionManager::setFlightMode_(bool enable) {
  digitalWrite(MODEM_FLIGHT, enable ? LOW : HIGH);
  delay(500);
}

bool LteConnectionManager::connectSimAndNetwork_() {
  Serial.println("SIM status...");
  SimStatus s = modem_.getSimStatus();

  if (s == SIM_LOCKED) {
    Serial.print("Unlocking with PIN ");
    Serial.print(USER_PIN_CODE);
    if (!modem_.simUnlock(USER_PIN_CODE)) {
      Serial.println("\nFATAL: SIM unlock failed");
      return false;
    }
    Serial.println(" ok");
    s = modem_.getSimStatus();
  }

  if (s != SIM_READY) {
    Serial.print("FATAL: SIM not ready. Status=");
    Serial.println((int)s);
    return false;
  }
  Serial.println("SIM ready");

  Serial.println("Waiting LTE attach...");
  digitalWrite(LED_PIN, HIGH);
  if (!modem_.waitForNetwork(NET_WAIT_MS)) {
    digitalWrite(LED_PIN, LOW);
    Serial.println("FATAL: LTE attach failed");
    isConnected_ = false;
    return false;
  }
  isConnected_ = true;
  digitalWrite(LED_PIN, LOW);
  Serial.println("LTE registered");
  Serial.print("Operator: ");
  Serial.println(modem_.getOperator());
  return true;
}

bool LteConnectionManager::startPdp_() {
  Serial.print("PDP/APN: ");
  Serial.print(APN);
  Serial.println(" ...");
  for (int i = 0; i < MAX_RETRIES; ++i) {
    if (modem_.gprsConnect(APN, GPRS_USER, GPRS_PASS)) {
      Serial.println("GPRS connected");
      Serial.print("Local IP: ");
      Serial.println(modem_.getLocalIP());
      return true;
    }
    Serial.print("Retry ");
    Serial.println(i + 1);
    delay((i + 1) * 2000);
  }
  Serial.println("FATAL: PDP failed");
  return false;
}

String LteConnectionManager::buildTelemetryJson_(const Telemetry& t) {
  StaticJsonDocument<1024> doc;

  JsonArray data = doc.createNestedArray("data");
  JsonObject entry = data.createNestedObject();

  entry["cow_id"] = t.cowId;
  entry["base_id"] = t.baseId;
  entry["event_type"] = "telemetry";

  JsonObject ev = entry.createNestedObject("event_data");
  ev["latitude"] = t.latitude;
  ev["longitude"] = t.longitude;
  ev["node_temperature"] = t.nodeTemperature;
  ev["node_battery"] = t.nodeBattery;
  ev["node_battery_percent"] = t.nodeBatteryPercent;
  ev["base_battery"] = t.baseBattery;
  ev["base_battery_percent"] = t.baseBatteryPercent;
  ev["isAlerted"] = t.isAlerted;
  ev["alertType"] = t.alertType;
  ev["node_vbus"] = t.nodeVbus;
  ev["node_has_battery"] = t.nodeHasBattery;

  entry["name"] = t.name;
  entry["tag_id"] = t.tagId;
  entry["birth_date"] = t.birthDate;
  entry["breed"] = t.breed;
  entry["id"] = t.cowId;

  String out;
  serializeJson(doc, out);
  return out;
}
