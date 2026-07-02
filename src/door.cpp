#include "door.h"

#include <Arduino.h>

namespace {

constexpr uint8_t kRelayPin = 26;

// Most cheap opto-isolated 1-channel relay boards trigger the relay when
// the signal pin is driven LOW, not HIGH. If your relay clicks on when you
// call open() and immediately clicks back off (or never clicks at all),
// flip this constant.
constexpr bool kRelayActiveLow = true;

bool holding_ = false;
uint32_t releaseAtMs_ = 0;

void energize() { digitalWrite(kRelayPin, kRelayActiveLow ? LOW : HIGH); }
void deenergize() { digitalWrite(kRelayPin, kRelayActiveLow ? HIGH : LOW); }

}  // namespace

namespace Door {

void begin() {
  pinMode(kRelayPin, OUTPUT);
  deenergize();  // boot-safe default: door NOT held open
  holding_ = false;
}

void open(uint8_t seconds) {
  energize();
  holding_ = true;
  releaseAtMs_ = millis() + static_cast<uint32_t>(seconds) * 1000UL;
}

void update() {
  if (holding_ && static_cast<int32_t>(millis() - releaseAtMs_) >= 0) {
    deenergize();
    holding_ = false;
  }
}

bool isOpen() { return holding_; }

}  // namespace Door
