#pragma once
#include <Arduino.h>

// --------- MODEM + LIBS ----------
#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>
#include <TinyGsm.h>

#include <ArduinoJson.h>

// --------- PINS / UART ----------
static constexpr uint32_t UART_BAUD = 115200;
static constexpr int MODEM_TX = 27;
static constexpr int MODEM_RX = 26;
static constexpr int MODEM_PWRKEY = 4;
static constexpr int MODEM_DTR = 32;
static constexpr int MODEM_FLIGHT = 25;
static constexpr int LED_PIN = 12;

// --------- NETWORK / API ----------
static constexpr const char* APN = "internet";
static constexpr const char* GPRS_USER = "";
static constexpr const char* GPRS_PASS = "";
static constexpr const char* USER_PIN_CODE = "1557";

static constexpr const char* API_SERVER = "vaquinet-api.onrender.com";
static constexpr int API_PORT = 443;
static constexpr const char* POST_TELEMETRY_ENDPOINT = "/cows/telemetry/batch";
static constexpr const char* GET_COWS_ENDPOINT = "/cows";
static constexpr const char* GET_ORDERS_ENDPOINT = "/orders";
static constexpr const char* API_KEY = "my_secure_api_key_12345";  // unused

// --------- RETRIES / TIMEOUTS ----------
static constexpr int MAX_RETRIES = 5;
static constexpr long NET_WAIT_MS = 120000L;
static constexpr uint16_t SSL_RX_BUF = 2048;
static constexpr uint16_t SSL_TX_BUF = 1024;
static constexpr uint32_t SSL_SESSION_SEC = 300;
static constexpr uint32_t HTTP_RESP_TIMEOUT = 5000;
