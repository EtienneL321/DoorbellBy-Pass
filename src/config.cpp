#include "config.h"

#include <Preferences.h>

namespace
{

  Preferences prefs;

  constexpr uint8_t kDefaultDurationSec = 5;
  constexpr uint8_t kMinDurationSec = 1;
  constexpr uint8_t kMaxDurationSec = 30; // hard cap regardless of requested value
  constexpr uint8_t kDefaultChimeTrack = 1;

} // namespace

namespace Config
{

  void begin() { prefs.begin("doorbell", /*readOnly=*/false); }

  uint8_t getDoorDuration()
  {
    return prefs.getUChar("duration", kDefaultDurationSec);
  }

  void setDoorDuration(uint8_t seconds)
  {
    if (seconds < kMinDurationSec)
      seconds = kMinDurationSec;
    if (seconds > kMaxDurationSec)
      seconds = kMaxDurationSec;
    prefs.putUChar("duration", seconds);
  }

  bool isMuted() { return prefs.getBool("muted", false); }

  void setMuted(bool muted) { prefs.putBool("muted", muted); }

  uint8_t getChimeTrack()
  {
    return prefs.getUChar("track", kDefaultChimeTrack);
  }

  void setChimeTrack(uint8_t track) { prefs.putUChar("track", track); }

} // namespace Config
