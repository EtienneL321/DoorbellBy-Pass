#pragma once

// Ring-button detection via the optocoupler on the buzzer wires.
// Optocoupler output pulls the input pin LOW while the intercom energizes
// the buzzer (i.e. while the button is held outside).
namespace Ring {

void begin();

// Must be called every loop() iteration.
void update();

// Fired once per debounced, rate-limited ring event (not once per loop
// while the button is held).
void onRing(void (*callback)());

}  // namespace Ring
