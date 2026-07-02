#include "chime.h"

#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

namespace {

// ESP32 hardware UART2. RX2 <- DFPlayer TX, TX2 -> DFPlayer RX.
constexpr int8_t kRxPin = 16;
constexpr int8_t kTxPin = 17;
constexpr uint8_t kDefaultVolume = 20;  // 0-30

HardwareSerial dfSerial(2);
DFRobotDFPlayerMini player;
bool ready_ = false;

}  // namespace

namespace Chime {

void begin() {
  dfSerial.begin(9600, SERIAL_8N1, kRxPin, kTxPin);
  ready_ = player.begin(dfSerial, /*isACK=*/true, /*doReset=*/true);
  if (ready_) {
    player.volume(kDefaultVolume);
  } else {
    Serial.println("Chime: DFPlayer not detected, chime disabled");
  }
}

void play(uint8_t track) {
  if (!ready_) return;
  player.play(track);
}

}  // namespace Chime
