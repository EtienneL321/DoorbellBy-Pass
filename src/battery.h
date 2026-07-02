#pragma once

// Battery voltage sensing via a resistor divider into an ADC pin, plus a
// latched low-battery flag for the Telegram alert.
namespace Battery {

void begin();

// Approximate cell voltage. Depends on the divider ratio wired in Phase 4 —
// see the comment in battery.cpp before trusting the absolute number.
float voltageV();

// True once voltage drops below the low threshold; stays true until
// voltage recovers above the threshold + hysteresis (avoids flapping).
bool isLow();

}  // namespace Battery
