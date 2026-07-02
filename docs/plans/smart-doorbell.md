# Plan: Smart Doorbell Retrofit (DoorbellBy-Pass)

## Progress

Check off each phase as it completes and commit — git history then doubles as
a project log. Each phase's *"Done when"* criteria (below) define completion.

- [ ] **Phase 0** — Discovery: intercom measured, assumptions confirmed
  *(buzzer AC/DC still needs a recheck — only blocks Phase 4 wiring, not
  Phase 1 ordering; see below)*
- [ ] **Phase 1** — Parts ordered and received
  *(all ordered; on hand now: ESP32 dev board, relay module, resistor
  assortment kit, breadboard + jumper wires. In transit via Amazon/Digikey,
  expected next week: DFPlayer Mini, speaker, PC817 optocoupler, bridge
  rectifier + cap, 18650 holder, MT3608 boost converter, switch. Enough on
  hand to start Phase 2 steps 1–4 — WiFi/Telegram bring-up and relay
  test — before the rest arrives.)*
- [ ] **Phase 2** — Bench prototype: full loop works with simulated ring
  *(in progress — PlatformIO project + full firmware written: config, door,
  ring, chime, battery, bot modules, see src/. Untested against real
  hardware/build yet — no PlatformIO toolchain available to compile-check
  while writing it. Testable today with on-hand parts: WiFi/Telegram
  bring-up (no hardware needed beyond the ESP32), relay open via `/open`
  or the notification button, and the full ring→notify→button→door flow by
  jumpering GPIO27 to GND to fake a ring — see chat for exact steps. Chime
  and battery reads safely no-op/read garbage until their hardware arrives
  next week.)*
- [ ] **Phase 3** — Firmware complete: all commands, ring flow, robustness
- [ ] **Phase 4** — Final circuit built in enclosure
- [ ] **Phase 5** — Installed at the callbox
- [ ] **Phase 6** — All verification checks pass

**Key measurements from Phase 0** (fill in — later phases depend on these):
- Buzzer signal: `18 V`, `AC / DC — TBD`: set multimeter explicitly to AC volts
  and remeasure while someone presses the button; ~0V there but 18V on DC
  means DC, and vice versa. This value sizes the Phase 4 rectifier/resistor —
  **not required until Phase 4**, so it doesn't block ordering parts (Phase 1)
  or bench prototyping (Phase 2).
  No multimeter handy? A rough no-meter test: wire a small LED + ~1kΩ resistor
  in series across the buzzer wires (either polarity), have someone press the
  button. If the LED lights clearly regardless of which way you connect it,
  it's AC. If it only lights in one orientation (or not at all reversed),
  it's DC. Treat this as a hint, not a substitute — confirm with a real
  multimeter (or ask to borrow one, most hardware stores/neighbors have one)
  before finalizing the Phase 4 circuit.
- Button is dry contact, short opens door: `yes` — confirmed
- WiFi at callbox: `good` — confirmed
- Outlet within reach: `no` — confirmed → device is **battery-powered**
  (single 18650, hot-swap), see Design decisions

## Overview

Retrofit the apartment's existing analog intercom with a small ESP32-based device
mounted near/inside the in-unit callbox. The device sits **in parallel** with the
existing hardware — a relay mimics holding the "open door" button, an optocoupler
senses the ring signal on the buzzer wires — so the original intercom keeps
working untouched and the mod is fully reversible. A Telegram bot provides the
phone interface: ring notifications with a one-tap "Open Door" button, remote
open from anywhere, and a configurable door-hold duration. A DFPlayer Mini +
small speaker replaces the harsh buzz with any MP3 melody. No hub, no
subscription: the ESP32 makes outbound-only connections, so it works from any
network with zero port forwarding.

## Requirements

1. **Remote door open** — open the building door from the phone, whether home or
   away (internet-reachable, not LAN-only).
2. **Ring notification** — push notification on the phone when someone presses
   this unit's callbox button.
3. **Configurable open duration** — the door release only engages while the
   button is held, so the device must hold it for N seconds, with N settable
   from the phone.
4. **Custom chime** — replace the buzzing sound with a pleasant melody.
5. **Small form factor** — fits near or inside the in-unit callbox.

Requirements 1–5 were given directly. Design choices under them (ecosystem,
chime hardware) were resolved through discussion — see Design decisions.

## Background knowledge

### How old apartment intercoms work
Most pre-2000s buildings use a simple analog multi-wire system. Each unit's
wall station has (at minimum):
- A **door-release button**: pressing it closes a circuit that energizes the
  building door's electric strike. It's usually a simple dry contact — two
  wires that get shorted together while pressed.
- A **buzzer/sounder**: when someone presses your unit's number outside, the
  panel puts a voltage (typically 9–24V, **either AC or DC** depending on the
  system) across your buzzer's two wires for as long as they hold the button.

That's the whole interface we need: *short two wires to open the door; detect
voltage on two other wires to know someone rang.* Everything is low-voltage —
no mains inside the callbox — but measure before touching (Phase 0).

**Caveat:** a minority of systems (mostly newer) are digital/multiplexed, where
the button sends a coded signal instead of a dry contact. Phase 0 includes a
test to rule this out before buying parts.

### ESP32
A ~$8 WiFi microcontroller board (~25×50mm), programmed in C++ via
Arduino/PlatformIO. 3.3V logic — its GPIO pins must never see the intercom's
9–24V directly, which is why both interfaces below are isolated. Runs the
firmware: WiFi, Telegram polling, relay timing, ring detection, chime playback.

### Relay module (door open)
An opto-isolated 1-channel 3.3V relay board (~$2). Its output contacts wire in
parallel with the existing door button — closing the relay is electrically
identical to pressing the button. The ESP32 energizes it for N seconds.

### Optocoupler input (ring detection)
A PC817 optocoupler (~$0.20) electrically isolates the intercom's buzzer
voltage from the ESP32. Buzzer voltage lights the opto's internal LED (through
a current-limiting resistor); the output side pulls an ESP32 GPIO low. If the
buzzer signal is AC (common), add a small bridge rectifier + smoothing
capacitor in front so the opto sees steady DC. The exact resistor value depends
on the voltage measured in Phase 0.

### DFPlayer Mini (chime)
A ~$3 module that plays MP3s from a microSD card and drives a small 4Ω/3W
speaker directly. The ESP32 commands it over serial UART ("play track 1").
Any melody = any MP3 you load on the card.

### Telegram Bot API
Create a bot via @BotFather (free, 2 minutes) → get a token. The ESP32 uses
the `UniversalTelegramBot` Arduino library to **long-poll** Telegram's servers
(outbound HTTPS only — works behind any router/NAT with no setup). The bot can
send you messages (ring notifications) with **inline keyboard buttons** ("Open
Door") and receive your commands (`/open`, `/duration 7`, `/status`). Security
comes from allowlisting your Telegram chat ID — the bot ignores everyone else.

### ESP32 Preferences (NVS)
Built-in key-value flash storage. Persists the configured door-hold duration
(and mute flag) across power cycles.

## Design decisions

1. **Microcontroller: ESP32** — Why: requirement 5 (small) rules out a
   Raspberry Pi; ESP32 has built-in WiFi, costs ~$8, and is the standard for
   this class of project. Rejected: RPi (size, power, overkill), ESP8266
   (viable but fewer pins/less headroom for TLS + audio for the same price).

2. **Door open: relay in parallel with the existing button** — Why:
   electrically identical to a physical press, additive and reversible
   (renter-friendly), physical button keeps working as a fallback. Rejected:
   replacing the button wiring (invasive, no fallback).

3. **Ring detection: optocoupler across the buzzer wires** — Why: clean,
   isolated, reliable digital signal. Rejected: microphone/vibration sensor
   listening for the buzz (non-invasive but false-triggers and misses rings).

4. **Ecosystem: custom ESP32 firmware + Telegram bot** — *Chosen by user.*
   Why: $0/month, no always-on hub, works from anywhere with outbound-only
   connections, and the firmware is a well-scoped DIY project. Rejected:
   ESPHome + Home Assistant (most polished daily UX but requires a hub device
   and Nabu Casa/VPN for remote access); hosted platforms like Blynk (vendor
   lock-in, free-tier limits).

5. **Telegram over ntfy** — Why: inline keyboard puts "Open Door" directly on
   the ring notification, chat commands make a natural config UI, and
   `UniversalTelegramBot` is mature on ESP32. ntfy remains a lighter-weight
   alternative if Telegram polling latency (~1–4s) ever grates.

6. **Chime: DFPlayer Mini + small speaker; original buzzer disconnected** —
   *Chosen by user.* Why: plays any MP3, real audio quality, $3. The original
   buzzer's wires are labeled and capped, not cut — reconnectable anytime.
   Rejected: ESP32-generated piezo tones (simpler but chiptune-quality);
   phone-notification-only (misses rings when phone is silent).

7. **Power: single 18650 Li-ion cell, hot-swappable** — *Chosen by user after
   Phase 0 found no outlet near the callbox.* A 3.7V→5V boost converter
   (e.g. MT3608) feeds the ESP32/relay/DFPlayer rail; the cell sits in a
   holder with no charging circuitry inside the enclosure. Why: smallest and
   simplest build (requirement 5) — no charge-controller IC, no exposed port,
   nothing that can overheat unattended inside a wall-mounted box. Swap the
   depleted cell for a spare, charge externally. Fits the user's existing
   habit of owning and recharging 18650s. Rejected: two cells in parallel
   (roughly doubles runtime but enlarges the enclosure, working against
   requirement 5 — revisit if single-cell runtime proves impractically
   short); onboard USB charging via TP4056 (doesn't clearly help — there's no
   outlet nearby either way, so charging in place just trades a cell-swap
   chore for a cable-run chore, while adding a charge IC and exposed port
   inside the enclosure).

   **Power budget:** ESP32 WiFi + Telegram long-polling draws ~120–260mA
   average. At that rate a single 18650 (~2,500–3,500mAh) lasts roughly
   1–3 days continuously awake. Two mitigations, both included in Phase 3:
   - **WiFi modem-sleep** (`WIFI_PS_MIN_MODEM`) — ESP32 stays associated to
     WiFi (so it doesn't miss a ring or a Telegram command) but sleeps the
     radio between beacon intervals. Meaningful power savings with no
     responsiveness cost — default on, not something to weigh trade-offs on.
   - **Low-battery alert** — a voltage divider on an ADC pin reports cell
     voltage; below ~3.5V the firmware sends one Telegram warning so a swap
     happens before the device goes dark (important since R1 requires the
     device to be reachable while you're away).
   Expect to establish a real swap cadence empirically in Phase 6 rather than
   trusting the estimate above.

8. **Security posture** — Telegram chat-ID allowlist (bot ignores all other
   users); bot token + WiFi credentials in a git-ignored `secrets.h`; firmware
   caps door-hold duration at 30s regardless of configured value.

## Implementation steps

### Phase 0 — Discovery: understand your specific intercom
*Goal: confirm the analog assumptions and get the numbers that size two components.*

1. Power nothing off yet — take photos of the callbox exterior, then open it
   (usually 1–2 screws) and photograph the wiring before touching anything.
2. Identify the wire pairs: door-button pair and buzzer pair (follow them from
   the button and sounder). Label them with tape.
3. **Button test:** with a multimeter on continuity, confirm the button is a
   dry contact (beeps closed when pressed). Then briefly short the two button
   wires with a jumper while a helper watches the door — if the door releases,
   the relay approach is confirmed. If not, the system may be digital → stop
   and reassess (see Open questions).
4. **Buzzer measurement:** have a helper press your unit's button outside while
   you measure across the buzzer wires. Record: AC or DC, and the voltage.
   This sets the optocoupler's series resistor and whether the bridge
   rectifier is needed.
5. Check WiFi signal strength at the callbox location with your phone.
6. Note available power: is there an outlet within cable reach?

*Done when:* you know button = dry contact, buzzer voltage/type, WiFi is
adequate, and power is available.

### Phase 1 — Parts (~$25 total)

| Part | ~Price | Purpose |
|---|---|---|
| ESP32 dev board (ESP32-WROOM DevKit) | $8 | Brain |
| 1-ch 3.3V opto-isolated relay module | $2 | Door open |
| PC817 optocoupler + resistor assortment kit | $1 | Ring detect |
| Bridge rectifier (e.g. DB107) + 10–47µF cap | $1 | Ring detect — buy regardless of AC/DC; costs $1 either way and only gets installed if the recheck confirms AC (harmless to have on hand if not needed) |
| DFPlayer Mini + microSD card (any small one) | $5 | Chime |
| 4Ω 3W mini speaker | $3 | Chime |
| 18650 cell holder (single, with leads) | $2 | Power — use your existing 18650s |
| MT3608 boost converter (3.7V→5V) module | $2 | Power |
| Small SPST slide/toggle switch | $1 | Power on/off (swap safety) |
| Resistors for battery voltage divider (2x, e.g. 100kΩ+100kΩ) | — | Low-battery ADC sense |
| Breadboard + jumper wires (prototyping) | — | Likely already owned |
| Small project box / 3D-printed enclosure | $0–5 | Requirement 5 |

### Phase 2 — Bench prototype (no intercom involved)
1. Create the PlatformIO project in this repo (`platformio.ini`, `src/main.cpp`);
   add `UniversalTelegramBot`, `ArduinoJson`, `DFRobotDFPlayerMini` deps.
2. Add `include/secrets.h` (WiFi creds, bot token, chat ID) and git-ignore it;
   commit a `secrets.example.h`.
3. Flash blink → WiFi connect → Telegram "hello": bot replies to `/status`.
4. Wire the relay on the breadboard; `/open` clicks it for 5s.
5. Wire the DFPlayer (UART2: GPIO16/17) + speaker; play a test MP3.
6. Simulate a ring: jumper the opto input GPIO → bot sends notification with
   inline "Open Door" button; tapping it fires the relay; melody plays.

*Done when:* the full loop works end-to-end on the bench with a simulated ring.

### Phase 3 — Firmware (the real build)
Suggested structure:
```
src/main.cpp          — setup + loop
src/door.{h,cpp}      — relay control: openDoor(seconds), non-blocking timer
src/ring.{h,cpp}      — ring input: debounce (~50ms), edge detect, rate-limit
src/chime.{h,cpp}     — DFPlayer wrapper: play(track), mute flag
src/bot.{h,cpp}       — Telegram: polling, command routing, chat-ID allowlist
src/config.{h,cpp}    — Preferences: duration (default 5s, max 30s), mute
src/battery.{h,cpp}   — ADC read + divider math, low-battery threshold check
```
Behavior:
- **On ring:** debounced rising edge → play melody (unless muted) → send
  Telegram message "🔔 Someone's at the door" with inline `[Open Door]` button.
  Rate-limit notifications (e.g. max 1 per 10s) so a held button doesn't spam.
- **Commands:** `/open` (open now), `/duration N` (set + persist, clamp 1–30),
  `/status` (uptime, WiFi RSSI, current duration, mute state, battery voltage),
  `/mute`, `/unmute`, `/chime N` (select track).
- **Power management:** WiFi modem-sleep (`WIFI_PS_MIN_MODEM`) enabled at
  boot — stays associated so rings/commands aren't missed, sleeps the radio
  between beacons to stretch battery life. Battery voltage polled periodically
  via ADC; below ~3.5V, send one Telegram low-battery warning (don't repeat
  every loop — flag once, clear on recharge/reboot).
- **Robustness:** WiFi auto-reconnect; relay defaults open (door NOT held) on
  boot/crash; watchdog reset.

*Done when:* all commands work; ring → notification latency under ~5s; device
recovers from WiFi drop and power cycle without losing settings.

### Phase 4 — Input conditioning + final circuit
1. Build the ring-detect front end per Phase 0 measurements: series resistor
   sized for the measured voltage (target ~10–20mA through the PC817 LED);
   bridge rectifier + cap first if AC.
2. Verify on the bench with a similar supply before connecting to the intercom.
3. Move from breadboard to soldered protoboard; mount ESP32, relay, opto
   circuit, DFPlayer in the enclosure with screw terminals for the four
   intercom wires (2 button, 2 buzzer) — screw terminals keep the install
   solder-free at the callbox end.

### Phase 5 — Install
1. Power down nothing (intercom stays live — it's low voltage; just avoid
   shorting stray wires).
2. Connect relay terminals in parallel with the button wires; connect the opto
   input across the buzzer wires; disconnect the original buzzer and cap +
   label its wires.
3. Mount the enclosure inside/beside the callbox with a charged 18650 seated
   and the power switch accessible without fully removing the enclosure (so
   swaps are quick).
4. End-to-end test with a helper (see Phase 6).

### Phase 6 — Verification (maps to requirements)
- [ ] **R1:** `/open` and the notification's "Open Door" button release the
      building door — tested on WiFi *and* on mobile data (away simulation).
- [ ] **R2:** Helper presses unit button outside → phone notification within ~5s.
- [ ] **R3:** `/duration 10` → door verifiably stays released ~10s; setting
      survives a power cycle.
- [ ] **R4:** Ring plays the chosen MP3, no buzz.
- [ ] **R5:** Everything fits in the enclosure at/inside the callbox,
      including the 18650 holder and boost converter.
- [ ] **Fallback:** original physical button still opens the door with the
      device powered off / cell removed.
- [ ] **Battery:** measure actual runtime on a full charge under normal use;
      record it below and set a swap reminder at ~80% of that interval.
      Confirm the low-battery Telegram alert fires before the device dies.

**Measured battery runtime:** `___ days` (fill in after a week of real use)

## Open questions / risks

- **Buzzer voltage unknown until Phase 0** — determines the rectifier and
  resistor. Architectural risk only if the system turns out digital.
- **Digital/multiplexed intercom (unlikely in an old building):** if the Phase
  0 button-short test fails, the relay approach won't work as-is; fallback is
  a small servo/solenoid physically pressing the button (clunky but universal),
  or identifying the system model for a protocol-level approach. Decide then.
- **WiFi coverage at the callbox** — if weak, options: reposition antenna-style
  dev board, WiFi extender, or mount the device farther from the callbox with
  longer low-voltage wires (the four intercom wires can be extended freely).
- **Telegram polling latency (~1–4s)** — acceptable for a doorbell; if it ever
  bothers you, ntfy with a persistent connection is the swap.
- **Security:** remote door-open is a real attack surface. Mitigations in the
  plan: chat-ID allowlist, 30s max hold, secrets never committed. Residual
  risk: Telegram account compromise = door access — keep 2FA on Telegram.
- **Lease/landlord:** the mod is reversible and confined to your unit's
  callbox, but worth a heads-up depending on your lease.

## Future goals

Not part of this build — deliberately deferred so a first project stays
simple. Recorded here so the current design doesn't make it harder to get to
later.

- **Migrate off Telegram to a local-only, self-hosted setup.** Once
  comfortable with this build and ready to keep going with home automation:
  add a Raspberry Pi as a hub running Tailscale or WireGuard, with the ESP32
  talking to it over LAN and your phone reaching it through the VPN mesh —
  removing Telegram (and any third party) from the loop entirely. This was
  discussed as an alternative during initial design (see chat) and is the
  natural "no compromises" end state of the reachability trade-offs made
  there.
- **Why deferred, not designed in now:** this is a first electronics/firmware
  project — Telegram gets to a working, reliable device fastest, without
  taking on Pi provisioning, VPN setup, and a second protocol at the same
  time. Revisit once the current build is solid and installed.
- **What it would touch when the time comes:** the firmware's modular split
  (`bot.cpp` isolated from `door`/`ring`/`chime`/`config`/`battery`) was
  chosen partly so this swap stays contained — replacing Telegram with a
  local HTTP server or MQTT client talking to the Pi should mean rewriting
  `bot.{h,cpp}` and its call sites in `main.cpp`, not the rest of the
  firmware. The chat-ID allowlist would be replaced by VPN network
  membership as the access control.
- **Natural on-ramp to broader home automation:** the same Pi could run Home
  Assistant, making this migration double as the entry point into a wider
  smart-home setup rather than a one-off. Worth standing up the Pi/VPN (and
  optionally Home Assistant) as its own small project first, independent of
  this doorbell, before integrating the two.
