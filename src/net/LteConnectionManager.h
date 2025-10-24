#pragma once
#include <Arduino.h>
#include "config/NetConfig.h"
#include "model/Telemetry.h"

class ESP_SSLClient;  // fwd declare

class LteConnectionManager {
 public:
  LteConnectionManager();
  ~LteConnectionManager();

  void shutdown();
  void powerOffModem_();
  void begin();
  bool ensureConnected();
  bool postTelemetry(const Telemetry& t);
  void disconnect();

 private:
  void setupPins_();
  void powerOnModem_();

  void setFlightMode_(bool enable);
  bool connectSimAndNetwork_();
  bool startPdp_();
  String buildTelemetryJson_(const Telemetry& t);

 private:
  HardwareSerial serial_;
  TinyGsm modem_;
  TinyGsmClient netClient_;
  ESP_SSLClient* ssl_;
  bool isConnected_;
};
