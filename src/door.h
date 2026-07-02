#pragma once

#include <cstdint>

// Building-door relay. Mirrors holding the callbox's door-release button.
namespace Door
{

    void begin();

    // Energize the relay for `seconds`. Non-blocking — call update() from loop().
    void open(uint8_t seconds);

    // Must be called every loop() iteration; releases the relay once the
    // hold time elapses.
    void update();

    bool isOpen();

} // namespace Door
