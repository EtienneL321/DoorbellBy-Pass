#include "battery.h"

#include <Arduino.h>

namespace
{

  constexpr uint8_t kAdcPin = 34; // input-only ADC1 pin

  // Divider ratio for two equal (e.g. 100k + 100k) resistors from the 18650's
  // positive terminal to ADC_PIN to GND: Vbattery = 2 * Vadc.
  // If you use different resistor values, update kDividerRatio accordingly
  // (ratio = (R1 + R2) / R2, where R1 is battery-side, R2 is ground-side).
  constexpr float kDividerRatio = 2.0f;

  constexpr float kLowThresholdV = 3.5f;
  constexpr float kRecoverThresholdV = 3.6f; // hysteresis to avoid flapping

  bool lowLatched_ = false;

} // namespace

namespace Battery
{

  void begin()
  {
    analogSetPinAttenuation(kAdcPin, ADC_11db); // full range up to ~3.3V at the pin
  }

  float voltageV()
  {
    // analogReadMilliVolts() uses the ESP32's eFuse ADC calibration and is
    // more accurate than scaling raw analogRead() counts by hand.
    uint32_t mv = analogReadMilliVolts(kAdcPin);
    return (mv / 1000.0f) * kDividerRatio;
  }

  bool isLow()
  {
    float v = voltageV();
    if (v <= kLowThresholdV)
    {
      lowLatched_ = true;
    }
    else if (v >= kRecoverThresholdV)
    {
      lowLatched_ = false;
    }
    return lowLatched_;
  }

} // namespace Battery
