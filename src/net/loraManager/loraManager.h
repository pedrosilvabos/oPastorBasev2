#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "config/loraConfig.h"

// declare shared SPI buses (only once defined in .cpp)
extern SPIClass loraSPI;
extern SPIClass sdSPI;

class LoRaManager {
 public:
  bool begin();
  bool receive(String& out);
  bool send(const String& msg);
};
