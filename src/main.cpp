#include <Arduino.h>
#include "net/lteManager/lteConnectionManager.h"
#include "model/telemetry.h"
#include "esp_sleep.h"
#include "app/baseController.h"

LteConnectionManager lte;
BaseController app;

static void serviceLoRaFor(BaseController& app, uint32_t ms) {
  const uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    app.loopOnce();
    delay(2);
  }
}

void setup() {
  lte.begin();
  if (!app.begin()) {
    while (true) {
      delay(1000);
    }
  }
  app.attachLte(&lte);
  if (!lte.ensureConnected()) {
    Serial.println("Initial connect failed. Deep sleeping.");
    esp_sleep_enable_timer_wakeup(30ULL * 1000000ULL);
    esp_deep_sleep_start();
  }
  Serial.println("Setup complete");
}

void loop() {
  serviceLoRaFor(app, 2000);

  if (lte.ensureConnected()) {
    if (app.hasBatchReady()) {
      app.postBatchToCloud();  // posts real buffered lines
    }
  } else {
    delay(15000);
  }
  // Sleep gate: only when idle and nothing left to send/post
  if (app.readyToSleep()) {
    uint32_t ms = app.timeUntilNextSyncMs();
    // guard: if zero or tiny, skip sleep this turn
    if (ms > 1500) {
      lte.shutdown();  // power off modem cleanly
      Serial.printf("🌙 Deep sleep for %lu ms\n", (unsigned long)ms);
      esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000ULL);
      esp_deep_sleep_start();
    }
  }
}
