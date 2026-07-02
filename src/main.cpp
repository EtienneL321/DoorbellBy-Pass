#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "battery.h"
#include "bot.h"
#include "chime.h"
#include "config.h"
#include "door.h"
#include "ring.h"
#include "secrets.h"

namespace {

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  Serial.print("Connected, IP: ");
  Serial.println(WiFi.localIP());

  // Stay associated (so we don't miss a ring or a command) but sleep the
  // radio between beacons to cut average power draw. If this call fails to
  // compile against your installed arduino-esp32 core version, use
  // WiFi.setSleep(true) instead - simpler, slightly less power savings.
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
}

void handleRing() {
  if (!Config::isMuted()) {
    Chime::play(Config::getChimeTrack());
  }
  Bot::sendRingAlert();
}

void checkBattery() {
  static bool wasLow = false;
  bool isLow = Battery::isLow();
  if (isLow && !wasLow) {
    Bot::sendLowBatteryAlert();
  }
  wasLow = isLow;
}

}  // namespace

void setup() {
  Serial.begin(115200);

  Config::begin();
  Door::begin();
  Ring::begin();
  Chime::begin();
  Battery::begin();

  connectWiFi();
  Bot::begin();

  Ring::onRing(handleRing);

  Serial.println("Doorbell ready.");
}

void loop() {
  Ring::update();
  Door::update();
  Bot::update();
  checkBattery();
}
