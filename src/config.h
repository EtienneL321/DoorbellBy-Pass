#pragma once

#include <cstdint>

// Persisted settings (ESP32 NVS via Preferences). Survives reboot/power loss.
namespace Config {

void begin();

// Seconds the relay holds the door release. Clamped to [1, 30].
uint8_t getDoorDuration();
void setDoorDuration(uint8_t seconds);

bool isMuted();
void setMuted(bool muted);

// Which DFPlayer track number plays on a ring.
uint8_t getChimeTrack();
void setChimeTrack(uint8_t track);

}  // namespace Config
