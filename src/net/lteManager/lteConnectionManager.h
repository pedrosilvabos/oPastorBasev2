#pragma once
#include <Arduino.h>
#include "config/netConfig.h"
#include "model/telemetry.h"

class ESP_SSLClient;  // fwd declare

class LteConnectionManager {
 public:
  LteConnectionManager();
  ~LteConnectionManager();

  void begin();
  bool ensureConnected();
  bool postTelemetry(const Telemetry& t);
  bool sendSms(const String& number, const String& text);
  void disconnect();
  void shutdown();  // graceful flight-mode + power cut

 private:
  // power + pins
  void setupPins_();
  void powerOnModem_();
  void powerOffModem_();
  void setFlightMode_(bool enable);

  // boot/probe helpers
  bool probeAT_(uint32_t total_ms);       // retrying AT probe
  bool waitSimReady_(uint32_t total_ms);  // wait for +CPIN: READY

  // network bring-up
  bool connectSimAndNetwork_();
  bool startPdp_();

  // payload
  String buildTelemetryJson_(const Telemetry& t);

 private:
  HardwareSerial serial_;
  TinyGsm modem_;
  TinyGsmClient netClient_;
  ESP_SSLClient* ssl_ = nullptr;
  bool isConnected_ = false;
};
