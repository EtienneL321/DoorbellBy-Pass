#pragma once

// Telegram bot: command handling, ring notifications, chat-ID allowlist.
namespace Bot
{

    void begin();

    // Call every loop() iteration; internally rate-limited to avoid hammering
    // the Telegram API.
    void update();

    // Ring notification with an inline "Open Door" button.
    void sendRingAlert();

    void sendLowBatteryAlert();

} // namespace Bot
