#pragma once

#include <cstdint>

// DFPlayer Mini melody playback. If the DFPlayer isn't detected at begin()
// (e.g. hardware not connected yet), play() silently no-ops rather than
// blocking or crashing.
namespace Chime {

void begin();
void play(uint8_t track);

}  // namespace Chime
