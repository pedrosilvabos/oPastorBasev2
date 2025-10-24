#pragma once
#include <Arduino.h>

// T-Beam/SX1276 defaults. Adjust if needed.

#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_CS 5
#define LORA_RST -1
#define LORA_IRQ 32

#define VSPI 3
#define HSPI 2

// RF params
static const long LORA_FREQ_HZ = 868E6;
static const int LORA_SF = 7;             // 7..12
static const long LORA_BW = 125E3;        // 125/250/500 kHz
static const int LORA_CR = 5;             // 5=4/5 .. 8=4/8
static const int LORA_TX_POWER = 14;      // dBm
static const byte LORA_SYNC_WORD = 0x34;  // private network
