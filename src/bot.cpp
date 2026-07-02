// NOTE: field/method names below (messages[i].type, .query_id,
// sendMessageWithInlineKeyboard, answerCallbackQuery) match
// UniversalTelegramBot ~1.3.x from memory, but weren't verified against a
// real compile (no PlatformIO toolchain available while writing this).
// Check these against the actual installed library on your first `pio run`
// and let me know what breaks.

#include "bot.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include "battery.h"
#include "config.h"
#include "door.h"
#include "secrets.h"

namespace {

constexpr uint32_t kPollIntervalMs = 1500;  // don't poll faster than this

WiFiClientSecure securedClient_;
UniversalTelegramBot bot_(BOT_TOKEN, securedClient_);
uint32_t lastPollMs_ = 0;

bool isAllowed(const String& chatId) { return chatId == ALLOWED_CHAT_ID; }

void sendHelp(const String& chatId) {
  bot_.sendMessage(chatId,
                    "/open - open the door now\n"
                    "/duration N - set door-hold seconds (1-30)\n"
                    "/status - current settings\n"
                    "/mute or /unmute - toggle chime\n"
                    "/chime N - select chime track",
                    "");
}

void sendStatus(const String& chatId) {
  String msg = "Uptime: " + String(millis() / 1000) + "s\n";
  msg += "WiFi RSSI: " + String(WiFi.RSSI()) + " dBm\n";
  msg += "Door hold: " + String(Config::getDoorDuration()) + "s\n";
  msg += "Muted: " + String(Config::isMuted() ? "yes" : "no") + "\n";
  msg += "Chime track: " + String(Config::getChimeTrack()) + "\n";
  msg += "Battery: " + String(Battery::voltageV(), 2) + "V";
  bot_.sendMessage(chatId, msg, "");
}

void handleCommand(const String& chatId, const String& text) {
  if (text == "/start" || text == "/help") {
    sendHelp(chatId);
  } else if (text == "/open") {
    Door::open(Config::getDoorDuration());
    bot_.sendMessage(chatId,
                      "Door opened for " + String(Config::getDoorDuration()) + "s",
                      "");
  } else if (text.startsWith("/duration")) {
    int spaceIdx = text.indexOf(' ');
    int seconds = spaceIdx > 0 ? text.substring(spaceIdx + 1).toInt() : 0;
    if (seconds > 0) {
      Config::setDoorDuration(static_cast<uint8_t>(seconds));
      bot_.sendMessage(chatId,
                        "Door hold set to " + String(Config::getDoorDuration()) + "s",
                        "");
    } else {
      bot_.sendMessage(chatId, "Usage: /duration <1-30>", "");
    }
  } else if (text == "/status") {
    sendStatus(chatId);
  } else if (text == "/mute") {
    Config::setMuted(true);
    bot_.sendMessage(chatId, "Chime muted", "");
  } else if (text == "/unmute") {
    Config::setMuted(false);
    bot_.sendMessage(chatId, "Chime unmuted", "");
  } else if (text.startsWith("/chime")) {
    int spaceIdx = text.indexOf(' ');
    int track = spaceIdx > 0 ? text.substring(spaceIdx + 1).toInt() : 0;
    if (track > 0) {
      Config::setChimeTrack(static_cast<uint8_t>(track));
      bot_.sendMessage(chatId, "Chime track set to " + String(track), "");
    } else {
      bot_.sendMessage(chatId, "Usage: /chime <track number>", "");
    }
  }
  // Unknown text: ignore rather than reply, to avoid noise from stray messages.
}

void handleCallbackQuery(const String& chatId, const String& data,
                          const String& queryId) {
  if (data == "open") {
    Door::open(Config::getDoorDuration());
    bot_.answerCallbackQuery(queryId, "Door opened");
  } else {
    bot_.answerCallbackQuery(queryId, "");
  }
}

void pollUpdates() {
  int count = bot_.getUpdates(bot_.last_message_received + 1);
  for (int i = 0; i < count; i++) {
    String chatId = bot_.messages[i].chat_id;
    if (!isAllowed(chatId)) continue;  // silently ignore unauthorized chats

    if (bot_.messages[i].type == "callback_query") {
      handleCallbackQuery(chatId, bot_.messages[i].text, bot_.messages[i].query_id);
    } else {
      handleCommand(chatId, bot_.messages[i].text);
    }
  }
}

}  // namespace

namespace Bot {

void begin() {
  // Skips TLS certificate validation. Simplest option for a hobby build
  // talking to a single fixed, trusted host (api.telegram.org). Revisit
  // with certificate pinning if you want to harden this later.
  securedClient_.setInsecure();
  Serial.println("Telegram bot ready.");
}

void update() {
  if (millis() - lastPollMs_ < kPollIntervalMs) return;
  lastPollMs_ = millis();
  pollUpdates();
}

void sendRingAlert() {
  String keyboardJson = "[[{\"text\":\"Open Door\",\"callback_data\":\"open\"}]]";
  bot_.sendMessageWithInlineKeyboard(ALLOWED_CHAT_ID, "\xF0\x9F\x94\x94 Someone's at the door",
                                      "", keyboardJson);
}

void sendLowBatteryAlert() {
  bot_.sendMessage(ALLOWED_CHAT_ID,
                    "\xE2\x9A\xA0 Doorbell battery low, swap the 18650 soon", "");
}

}  // namespace Bot
