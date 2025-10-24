#pragma once
#include <Arduino.h>

struct Telemetry {
  const char* cowId;
  const char* baseId;
  const char* name;
  const char* tagId;
  const char* birthDate;
  const char* breed;

  float latitude;
  float longitude;
  float nodeTemperature;
  float nodeBattery;
  int nodeBatteryPercent;
  float baseBattery;
  int baseBatteryPercent;
  bool isAlerted;
  int alertType;
  int nodeVbus;
  int nodeHasBattery;
};

// exact values from your original code
inline Telemetry sampleTelemetry() {
  return Telemetry{"ESPCOW_cow_0",
                   "base_001",
                   "Madrugada",
                   "TAG001",
                   "2022-04-10",
                   "Holstein",
                   39.72999919f,
                   -27.0748558f,
                   31.69f,
                   3.98f,
                   50,
                   3.785f,
                   45,
                   false,
                   52,
                   0,
                   1};
}
