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
    (void)lte.postTelemetry(sampleTelemetry());
  } else {
    delay(15000);
  }

  lte.shutdown();

  Serial.println("Entering deep sleep for 30 seconds...");
  esp_sleep_enable_timer_wakeup(30ULL * 1000000ULL);
  esp_deep_sleep_start();
}
