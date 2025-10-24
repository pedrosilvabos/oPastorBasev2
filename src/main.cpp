#include "net/LteConnectionManager.h"
#include "model/Telemetry.h"
#include "esp_sleep.h"
#include "app/BaseController.h"

LteConnectionManager lte;
BaseController app;

// --- helper: give LoRa time to process inbound/outbound packets ---
static void serviceLoRaFor(BaseController& app, uint32_t ms) {
  const uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    app.loopOnce();  // non-blocking LoRa RX/TX + pairing window logic
    delay(2);        // yield briefly
  }
}

// --- standard Arduino entry points ---
void setup() {
  lte.begin();
  if (!app.begin())
    while (true) {
      delay(1000);
    }

  if (!lte.ensureConnected()) {
    Serial.println("Initial connect failed. Deep sleeping.");
    esp_sleep_enable_timer_wakeup(30ULL * 1000000ULL);
    esp_deep_sleep_start();
  }
  Serial.println("Setup complete");
}

void loop() {
  serviceLoRaFor(app, 2000);  // 2 s LoRa listen window

  if (lte.ensureConnected()) {
    (void)lte.postTelemetry(sampleTelemetry());
  } else {
    delay(15000);  // short backoff if failed
  }

  lte.shutdown();  // wrapper around powerOffModem_()

  Serial.println("Entering deep sleep for 30 seconds...");
  esp_sleep_enable_timer_wakeup(30ULL * 1000000ULL);
  esp_deep_sleep_start();
}
