# Handoff — stuck UPDATE READY takeover (2026-09-09)

> **CLOSED 2026-09-14.** The delivery below finally happened, carried by a
> later image: `v1.0.0-46-g70145f1` went over the air and the panel booted
> into `ota_1` running it (it had been on `v1.0.0-39-g9ec370b` in `ota_0`
> since 2026-09-05). That image contains this takeover fix plus
> `CONFIG_TORGET_IDLE_DIM=n` and the boot-screen teardown fix. Both open
> risks below are settled: the recovered `TG_OTA_TOKEN` was accepted (202,
> no 401), and the IDF 5.5.0 build passed the boot-health gate on hardware.
> Kept for the root-cause story; nothing here is still pending.

Everything is committed, pushed and green. **One step remains: deliver the
build to the panel.** Resume instructions are at the bottom.

---

## The bug (found, fixed, not yet on the device)

The panel sat on the UPDATE READY takeover with every touch dead, including
the UPDATE NOW pill.

Only the UPDATE pill means yes; all other glass snoozes. The snooze path
called `tg_notice_dismiss()`, which set `showing = false` and returned
`void` — so nothing ever told the overlay to hide. The guard hides only on
`TG_NOTICE_HIDE` from `tg_notice_update()`, and that call answers `NONE`
right after a dismiss (showing is already false, the nag clock just
restarted). The takeover stayed painted for good, and because *both* tap
branches are gated on `notice.showing`, the same stray touch that wedged the
glass also killed the pill that could have unwedged it.

Recovery was only ever KEY3 or a reboot — both bypass the notice policy.

**Fix:** `tg_notice_dismiss()` now returns `tg_notice_action` and the guard
obeys it. Commit `4c5b053`, CI green.

- `components/torget_ota/notice_policy.{c,h}` — dismiss returns the action
- `components/torget_ota/ota_service.c:454` — guard honours it
- `test/test_ota_notice_policy.c` — pins visible→`HIDE`, stray→`NONE`
- `docs/lessons.md`, `CHANGELOG.md`, `docs/ota.md` troubleshooting row

## Current state

| Thing | State |
|---|---|
| Panel | **Working.** Unstuck via KEY3 hold. Runs `v1.0.0-39-g9ec370b` |
| Firmware fix | Committed `4c5b053`, pushed to `fork`, **CI green** |
| Built binary | `build/torget.bin`, embeds `v1.0.0-43-g4c5b053` (clean) |
| Delivered to device? | **NO** — this is the remaining step |
| Tokenserver | Running (launchd), `otaAvailableVersion: None` |
| OTA sender | Stopped. Nothing is waiting to flash |

The panel will not nag again: this Mac is the only `_vibepulse._tcp` host on
the LAN and it announces nothing (the stale dirty `build/` was deleted).
The bug only resurfaces when something announces an update *and* you tap
outside the pill.

## What was repaired along the way

`~/esp/esp-idf`, `~/.espressif` and `secrets.h` had all been wiped — which is
why nothing had built since 2026-09-05 and why the tokenserver was stuck
advertising a stale `-dirty` binary that `ota-flash.sh` would always refuse.

- ESP-IDF **v5.5** reinstalled at `~/esp/esp-idf`
- `cmake` 3.30.2 + `ninja` 1.12.1 installed via `idf_tools.py` (scoped to the
  IDF install; Homebrew untouched)
- Stale `build/` removed — it had the old, deleted `/opt/homebrew/bin/cmake`
  baked into `build.ninja`
- `secrets.h` recovered from
  `~/Library/Mobile Documents/.Trash/secrets.h`, restored as a **copy**
  (chmod 600, git-ignored). **Back this file up somewhere permanent — the
  Trash copy will be purged.**

## Two open risks

1. **Token may not match.** The board runs the `-39` build; the recovered
   `secrets.h` is from the `-40` era. If `TG_OTA_TOKEN` was rotated between
   them the upload returns **401**. Safe failure: nothing is written.
2. **Built against IDF 5.5.0, not 5.5.2.** `dependencies.lock` records 5.5.2;
   the cloned `v5.5` tag is 5.5.0. The lock edit was reverted rather than
   record a false downgrade. Compiles cleanly, same minor. The boot-health
   gate rolls back automatically if the new image fails to bring up display,
   UI, scheduler, NVS and memory within 15 s.

To remove risk 2, install IDF 5.5.2 and rebuild before sending.

## Resume: deliver the update

```sh
cd ~/Documents/GitHub/vibepulse
. ~/esp/esp-idf/export.sh
idf.py build          # optional; build/torget.bin is already current
tools/ota-flash.sh    # waits; reads the IP from .ota-device (192.168.1.147)
```

Then **hold KEY3 ~3 s** on the panel. Delivery is automatic from there:
RECEIVING → VERIFYING → RESTARTING.

Gates are already satisfied — the version reads clean from the image and CI
on `4c5b053` is green. **Do not** set `TG_OTA_ALLOW_DIRTY` or
`TG_OTA_ALLOW_NO_CI`; they are not needed here.

Verify afterwards: the panel's version line should read
`v1.0.0-43-g4c5b053`.

### If the panel ever wedges on UPDATE READY again

Hold KEY3 ~3 s (repaints the glass), then short-press KEY3 to close. Or power
cycle. Tapping can never recover it on `-39` firmware.

## Verification actually performed

- All OTA host tests green under `-Wall -Wextra -Werror`: notice, button,
  request and boot-health policies, plus the 7 OTA wiring/python tests
- CI green on `4c5b053`
- Target build succeeds; embedded version clean
- **Not** verified: the fix running on hardware — it has not been delivered

Full `test/run.sh` aborts early at the interaction-relay crypto vectors
(needs ESP-IDF mbedtls). Pre-existing, unrelated.
