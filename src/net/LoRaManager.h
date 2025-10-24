#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "config/LoRaConfig.h"

SPIClass loraSPI(VSPI);
SPIClass sdSPI(HSPI);

class LoRaManager {
 public:
  bool begin() {
    loraSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    LoRa.setSPI(loraSPI);
    LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
    if (!LoRa.begin(LORA_FREQ_HZ)) return false;

    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setSignalBandwidth(LORA_BW);
    LoRa.setCodingRate4(LORA_CR);
    LoRa.setSyncWord(LORA_SYNC_WORD);
    LoRa.setTxPower(LORA_TX_POWER, PA_OUTPUT_PA_BOOST_PIN);
    LoRa.disableCrc();  // enable if your nodes use CRC
    return true;
  }

  // Non-blocking receive. Returns true and fills 'out' if a packet arrived.
  bool receive(String& out) {
    int psize = LoRa.parsePacket();
    if (psize <= 0) return false;
    out.reserve(psize);
    out = "";
    while (LoRa.available()) out += (char)LoRa.read();
    return true;
  }

  // Best-effort send. Returns true if queued to RF.
  bool send(const String& msg) {
    LoRa.beginPacket();
    LoRa.print(msg);
    return LoRa.endPacket() == 1;
  }
};
