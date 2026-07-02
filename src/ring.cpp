#include "ring.h"

#include <Arduino.h>

namespace
{

  constexpr uint8_t kRingPin = 27;
  constexpr uint32_t kDebounceMs = 50;
  constexpr uint32_t kRateLimitMs = 10000; // ignore re-triggers within 10s of the last

  void (*ringCallback_)() = nullptr;

  bool stableState_ = HIGH; // idle: pull-up holds the line HIGH
  bool lastReading_ = HIGH;
  uint32_t lastChangeMs_ = 0;
  uint32_t lastTriggerMs_ = 0;

} // namespace

namespace Ring
{

  void begin()
  {
    pinMode(kRingPin, INPUT_PULLUP);
    stableState_ = digitalRead(kRingPin);
    lastReading_ = stableState_;
  }

  void onRing(void (*callback)()) { ringCallback_ = callback; }

  void update()
  {
    bool reading = digitalRead(kRingPin);

    if (reading != lastReading_)
    {
      lastChangeMs_ = millis();
      lastReading_ = reading;
    }

    bool debounced = (millis() - lastChangeMs_) > kDebounceMs;
    if (debounced && reading != stableState_)
    {
      stableState_ = reading;

      if (stableState_ == LOW)
      { // falling edge: buzzer energized, someone is ringing
        uint32_t now = millis();
        if (now - lastTriggerMs_ >= kRateLimitMs)
        {
          lastTriggerMs_ = now;
          if (ringCallback_)
            ringCallback_();
        }
      }
    }
  }

} // namespace Ring
