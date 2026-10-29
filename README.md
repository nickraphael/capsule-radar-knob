# Capsule Radar 🛩️

<p align="center">
  <a href="https://socquique.github.io/capsule-radar/"><img src="https://img.shields.io/badge/Flash%20in%20browser-FF6D00?logo=googlechrome&logoColor=white" alt="Flash in browser"></a>
  <a href="https://makerworld.com/en/models/2907695-capsule-radar-live-flight-radar-desk-gadget"><img src="https://img.shields.io/badge/MakerWorld-3D%20case-1A8917?logo=bambulab&logoColor=white" alt="MakerWorld – 3D case"></a>
  <img src="https://img.shields.io/badge/board-ESP32--S3%20round%20AMOLED-E7352C?logo=espressif&logoColor=white" alt="Board: ESP32-S3 round AMOLED">
  <a href="https://github.com/socquique/capsule-radar/releases"><img src="https://img.shields.io/github/v/tag/socquique/capsule-radar?label=firmware&color=7B42BC" alt="Firmware version"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/code-MIT-2088FF" alt="License: MIT"></a>
  <img src="https://img.shields.io/github/languages/count/socquique/capsule-radar?label=languages&color=FFC107" alt="Languages">
  <a href="https://github.com/socquique/capsule-radar/stargazers"><img src="https://img.shields.io/github/stars/socquique/capsule-radar?style=social" alt="GitHub stars"></a>
</p>

<p align="center">
  <img src="docs/img/device.JPG" width="330" alt="Capsule Radar — a live flight on the device">
</p>
<p align="center"><sub>A real flight on the device: callsign, type, altitude/speed, <b>route</b> (Lisbon → Abu Dhabi) and the <b>aircraft photo</b> — all looked up automatically.</sub></p>

A live **ADS-B aircraft radar** for the **Waveshare ESP32-S3-Touch-AMOLED-1.75** — a round 466×466 AMOLED with capacitive touch. It pulls nearby aircraft from a free online feed over WiFi and plots them on a touch radar scope centered on your location, with live flight details and selectable visual skins.

> Visual reference: open [`assets/plane_radar_2.0_mockup.html`](assets/plane_radar_2.0_mockup.html) in a browser.

<p align="center"><img src="docs/img/radar.gif" width="360" alt="Capsule Radar live scope"></p>

| Phosphor | Orb | Amber CRT | Military |
|:--:|:--:|:--:|:--:|
| ![Phosphor](docs/img/radar.png) | ![Orb](docs/img/orb.png) | ![Amber](docs/img/amber.png) | ![Military](docs/img/military.png) |

<sub>Captured from the bundled desktop simulator (the device screen is round; the square corners are off-panel).</sub>

## Features

- **Live traffic** from [airplanes.live](https://airplanes.live), [adsb.fi](https://adsb.fi/) and [adsb.lol](https://adsb.lol) (free, non-commercial), updated every couple of seconds. Each provider is paced independently, so one refusing or rate-limiting does not stop the feed. Memory-safe streaming parser with a hard aircraft cap. *Note: airplanes.live is now [contributor-only](https://airplanes.live/get-started/) — if you run one of their feeders, your radar uses their API automatically from the same network (no keys, nothing to configure); otherwise it quietly falls back to adsb.fi / adsb.lol.*
- **Four themes** (long-press the screen to cycle, or pick on the web; remembered across reboots):
  - **Phosphor** — green-on-black radar scope: rings, animated sweep, aircraft glyphs rotated by heading and color-coded by altitude, fading trails, emergency halo.
  - **Orb** — green gradient + grid scope: the 7 nearest aircraft as yellow orbs emitting waves, off-range traffic as edge arrows pointing its way, orange target rings.
  - **Amber CRT** and **Military** — the same scope retinted (warm amber / night-vision green).
- **Touch** (CST9217): tap an aircraft → detail card (callsign, type, altitude, vertical speed, ground speed, distance, ground track (HDG), squawk, and **origin → destination** looked up from adsbdb, cached in NVS). The on-screen zoom button cycles the zoom range (10–100 km; the web page goes to 150 km). Swipe between **Radar / List / Stats** (circular layouts).
- **Boot splash** + **alert pings** (ES8311 speaker): a soft ping when a new aircraft enters range, an urgent double-beep for emergency/military — volume & mute on the web page.
- **Smooth motion**: aircraft glyphs glide between polls (interpolated) instead of jumping, using cheap partial redraws.
- **Top HUD**: WiFi status (amber if the data feed is failing), in-range aircraft count, NTP/RTC clock, **battery %** (charging bolt, red when low), and the date. The Stats view footer shows how to reach the config page (`capsuleradar.local` + IP).
- **Battery aware** (AXP2101): shows charge level, warns when low, and slows the feed poll rate on battery to save power.
- **Real-time clock** (PCF85063): keeps the time/date across power loss, so the clock is right even before/without WiFi; re-synced from NTP when online.
- **Smart brightness**: configurable idle auto-dim (no touch), and **face-down sleep** (QMI8658 IMU — flip it over to turn the screen off; a touch or turning it back up wakes it).
- **Idle clock**: instead of dimming, the radar turns into an analog clock when nobody touches it — the sweep line spins onto the current second and becomes the second hand, the hour and minute hands grow out of the centre, N/E/S/W become 12/3/6/9, and the planes keep moving behind it, dimmed. A touch brings the radar back (that touch does nothing else), and so does an emergency squawk in range. The web page's "Clock or dim after" setting sets the wait; a web switch turns the clock off.
- **GPS auto-location** (optional **-G** board variant): the Waveshare `-G` board has an onboard GPS (Quectel LC76G). Turn it on from the web page and the radar **sets its own center point automatically**, with an on-screen **satellite status icon** (amber while acquiring, green once it has a fix). Standard boards simply enter their location manually.
- **Configuration web page** at `http://capsuleradar.local/` — center point (map picker), display range, theme, **time zone** (auto-detected from your browser), live brightness slider, sound, WiFi reset, and over-the-air firmware update. Settings persist in NVS.
- **First-boot WiFi setup** via a captive portal (`CapsuleRadar-Setup`).

## Hardware

The reference board is the Waveshare **ESP32-S3-Touch-AMOLED-1.75**: ESP32-S3R8 (8 MB PSRAM, 16 MB flash), **CO5300** AMOLED over QSPI, **CST9217** touch, **QMI8658** IMU, **PCF85063** RTC, **AXP2101** PMIC, **ES8311** audio + speaker, microSD.

The Waveshare **ESP32-S3-Touch-AMOLED-1.43** is also supported in-tree. Same SoC, same 466×466 CO5300 panel — but a completely different pin map, an **FT3168** touch controller, and no PMIC or audio codec (so no battery readout and no alert pings; everything else works).

| | 1.75 (reference) | 1.43 |
|---|---|---|
| PlatformIO env | `esp32-s3-amoled-175` | `esp32-s3-amoled-143` |
| QSPI CS / SCLK / D0–D3 | 12 / 38 / 4,5,6,7 | 9 / 10 / 11,12,13,14 |
| LCD reset | 39 | 21 |
| I2C SDA / SCL | 15 / 14 | 47 / 48 |
| Touch | CST9217 @ 0x5A | FT3168 @ 0x38 |
| Battery / audio | AXP2101 / ES8311 | not fitted |

Pin maps live in [`src/boards/`](src/boards/), one header per board, selected by a `-DBOARD_*` build flag. They are taken from the vendor board definition and then **confirmed on hardware** — never guessed. Shared tunables stay in [`src/config.h`](src/config.h).

### Waveshare ESP32-S3-Touch-LCD-2.1 (experimental)

Contributed by @alexzogh in #45. **Experimental:** touch, WiFi, the feed and web updates work, but
the picture can shift sideways now and then, more during a web update
([#49](https://github.com/socquique/capsule-radar/issues/49)).

The 2.1-inch board uses a 480×480 ST7701 RGB IPS panel, CST820 touch, and a
TCA9554 GPIO expander. Its pin map and timings live in `src/boards/board_lcd_21.h`.
It has no PMIC or audio codec; battery reporting and alert sounds are disabled.

Build with `pio run -e esp32-s3-lcd-21`; upload over USB with `-t upload`, or use
`pio run -e esp32-s3-lcd-21-ota -t upload` once connected to WiFi.
Release assets include `CapsuleRadar-esp32s3-lcd21.bin` (merged USB image at 0x0)
and `CapsuleRadar-ota-lcd21.bin` (app-only image for the device's web update page).
The browser flasher currently supports the AMOLED boards.

The LCD environment rebuilds ESP-IDF with IRAM-safe LCD/GDMA interrupts and octal
PSRAM enabled for RGB refresh during WiFi flash operations. Its initial build takes
longer than the AMOLED environments. The QSPI sweep compositor remains enabled on
AMOLED; the RGB panel uses the shared LVGL sweep renderer.

### Where to buy the board

The 1.75″ board is in a **global stock squeeze** right now (even Waveshare lists it on backorder), so availability moves week to week. The official store is the reference; the Amazon links are what is actually in stock as of this writing.

| Board | Waveshare official (ships worldwide) | Amazon (affiliate links) |
|---|---|---|
| **1.75″ standard** — no case; the one the printed enclosure needs | [product page](https://www.waveshare.com/esp32-s3-touch-amoled-1.75.htm) (backorder) | 🇺🇸 [.com](https://www.amazon.com/dp/B0F7XTJ1ZL?tag=capsuleradar-20) · 🇪🇸 [.es](https://www.amazon.es/dp/B0F7XTJ1ZL?tag=capsuleradar-21) · 🇩🇪 [.de](https://www.amazon.de/dp/B0F7XTJ1ZL?tag=capsulerada03-21) · 🇬🇧 [.co.uk](https://www.amazon.co.uk/dp/B0F7XTJ1ZL?tag=capsulerada0d-21) · 🇮🇹 [.it](https://www.amazon.it/dp/B0F7XTJ1ZL?tag=capsulerada08-21) · 🇫🇷 [.fr](https://www.amazon.fr/dp/B0F7XTJ1ZL?tag=capsulerada0e-21) · 🇯🇵 [.co.jp](https://www.amazon.co.jp/dp/B0F7XTJ1ZL?tag=capsuleradar-22) · 🇨🇦 [.ca](https://www.amazon.ca/dp/B0F7XTJ1ZL?tag=capsulerada09-20) — official Waveshare listing, restocking |
| **1.75″ -G** — adds GPS (auto-location, heading-up) | same page, pick the **-G** option | 🇺🇸 [.com](https://www.amazon.com/dp/B0F7XWWMJW?tag=capsuleradar-20) · 🇪🇸 [.es](https://www.amazon.es/dp/B0F7XWWMJW?tag=capsuleradar-21) · 🇩🇪 [.de](https://www.amazon.de/dp/B0F7XWWMJW?tag=capsulerada03-21) · 🇬🇧 [.co.uk](https://www.amazon.co.uk/dp/B0F7XWWMJW?tag=capsulerada0d-21) · 🇮🇹 [.it](https://www.amazon.it/dp/B0F7XWWMJW?tag=capsulerada08-21) · 🇫🇷 [.fr](https://www.amazon.fr/dp/B0F7XWWMJW?tag=capsulerada0e-21) · 🇯🇵 [.co.jp](https://www.amazon.co.jp/dp/B0F7XWWMJW?tag=capsuleradar-22) · 🇨🇦 [.ca](https://www.amazon.ca/dp/B0F7XWWMJW?tag=capsulerada09-20) — official Waveshare listing, restocking · 🇺🇸 [third-party, in stock](https://www.amazon.com/dp/B0F8867487?tag=capsuleradar-20) |
| **1.75″ -B** — the same board inside Waveshare's own case. Works, but you must open that case yourself and its shell is **not** the printed enclosure | same page, pick the **-B** option | 🇺🇸 [.com](https://www.amazon.com/dp/B0F7XTJ7JW?tag=capsuleradar-20) · 🇪🇸 [.es](https://www.amazon.es/dp/B0F7XTJ7JW?tag=capsuleradar-21) · 🇩🇪 [.de](https://www.amazon.de/dp/B0F7XTJ7JW?tag=capsulerada03-21) · 🇬🇧 [.co.uk](https://www.amazon.co.uk/dp/B0F7XTJ7JW?tag=capsulerada0d-21) · 🇮🇹 [.it](https://www.amazon.it/dp/B0F7XTJ7JW?tag=capsulerada08-21) · 🇫🇷 [.fr](https://www.amazon.fr/dp/B0F7XTJ7JW?tag=capsulerada0e-21) · 🇯🇵 [.co.jp](https://www.amazon.co.jp/dp/B0F7XTJ7JW?tag=capsuleradar-22) · 🇨🇦 [.ca](https://www.amazon.ca/dp/B0F7XTJ7JW?tag=capsulerada09-20) |
| **1.43″** — the second supported board | [product page](https://www.waveshare.com/esp32-s3-touch-amoled-1.43.htm), pick **without case** | 🇪🇸 [.es](https://www.amazon.es/dp/B0DV3KM88S?tag=capsuleradar-21) · 🇩🇪 [.de](https://www.amazon.de/dp/B0DV3KM88S?tag=capsulerada03-21) · 🇬🇧 [.co.uk](https://www.amazon.co.uk/dp/B0DV3KM88S?tag=capsulerada0d-21) · 🇮🇹 [.it](https://www.amazon.it/dp/B0DV3KM88S?tag=capsulerada08-21) · 🇫🇷 [.fr](https://www.amazon.fr/dp/B0DV3KM88S?tag=capsulerada0e-21) · 🇯🇵 [.co.jp](https://www.amazon.co.jp/dp/B0DT9PSDW6?tag=capsuleradar-22) · 🇨🇦 [.ca](https://www.amazon.ca/dp/B0DT9PSDW6?tag=capsulerada09-20) — no US listing; use the Waveshare store there |
| **Battery** (optional) — 3.7 V LiPo, 1100 mAh, protection board, JST/MX1.25 plug (the one I run myself) | — | 🇪🇸 [.es](https://www.amazon.es/dp/B0F1FGZQS5?tag=capsuleradar-21) · 🇺🇸 [.com](https://www.amazon.com/dp/B0F1FGZQS5?tag=capsuleradar-20) · 🇮🇹 [.it](https://www.amazon.it/dp/B0F1FGZQS5?tag=capsulerada08-21) · 🇫🇷 [.fr](https://www.amazon.fr/dp/B0F1FGZQS5?tag=capsulerada0e-21) (DE/UK: that exact pack is not listed — search for a *protected* 3.7 V LiPo, ~1100 mAh, size **102540**, 1.25 mm micro-JST plug) — ⚠️ check the plug's **polarity** against the board's markings before connecting; it is not standardised across vendors |
| **Printed-parts hardware** — 6×2 mm disc magnets + M2×5 screws (counts per case variant on the MakerWorld page) | — | Magnets, the ones I used: 🇪🇸 [.es](https://www.amazon.es/dp/B0F4BZ447N?tag=capsuleradar-21) — elsewhere search "neodymium magnets 6x2mm": 🇺🇸 [.com](https://www.amazon.com/s?k=neodymium+magnets+6x2mm&tag=capsuleradar-20) · 🇩🇪 [.de](https://www.amazon.de/s?k=neodym+magnete+6x2mm&tag=capsulerada03-21) · 🇬🇧 [.co.uk](https://www.amazon.co.uk/s?k=neodymium+magnets+6x2mm&tag=capsulerada0d-21) · 🇮🇹 [.it](https://www.amazon.it/s?k=magneti+neodimio+6x2mm&tag=capsulerada08-21) · 🇫🇷 [.fr](https://www.amazon.fr/s?k=aimants+neodyme+6x2mm&tag=capsulerada0e-21) · 🇯🇵 [.co.jp](https://www.amazon.co.jp/s?k=neodymium+magnets+6x2mm&tag=capsuleradar-22) · 🇨🇦 [.ca](https://www.amazon.ca/s?k=neodymium+magnets+6x2mm&tag=capsulerada09-20). M2×5 screws: 🇪🇸 [.es](https://www.amazon.es/s?k=tornillos+M2x5&tag=capsuleradar-21) · 🇺🇸 [.com](https://www.amazon.com/s?k=M2x5+machine+screws&tag=capsuleradar-20) |

> ⚠️ **Do not buy the “1.75C”** (`ESP32-S3-Touch-AMOLED-1.75C`, aluminum case): despite the name it is a **different board** and this firmware does not run on it.

<sub>The Amazon links are affiliate links — as an Amazon Associate I earn from qualifying purchases, at no extra cost to you. The Waveshare links are plain links.</sub>

## Build & flash (PlatformIO)

```bash
pio run -e esp32-s3-amoled-175 -t upload     # build + flash over USB-C (1.75)
pio run -e esp32-s3-amoled-143 -t upload     # ...or the 1.43
pio device monitor -b 115200                  # serial log
```
The boot log names the board it was built for (`Capsule Radar boot  fw … board …`) — worth checking first if the screen stays black, since the wrong board's image drives the wrong pins.
On first flash you may need to hold **BOOT** then tap **RESET**. After flashing, on first boot connect your phone to the **`CapsuleRadar-Setup`** WiFi and enter your home network — real aircraft appear within seconds.

## Building on a locked-down Windows machine (Docker)

*Fork-specific.* Some managed Windows laptops block unsigned executables, so PlatformIO's GCC, `pip.exe` and the VS Code PlatformIO installer fail with "Access is denied". The workaround: compile inside a Linux container, where that policy doesn't apply, and run only the flashing on Windows (esptool is pure Python, run through the signed `python.exe`). If your machine has no such restriction, skip this section and use `pio run -e <env> -t upload` as above.

> **Status:** the knob env (`esp32-s3-knob-18`, set as `KNOB_ENV` in `settings.cmd`) is **not in `platformio.ini` yet**; it arrives with the port. Until then, `build.cmd` and `merge.cmd` accept any existing env as an argument (for example `.\build.cmd esp32-s3-amoled-175`), which is how to test the toolchain. `deploy.cmd` refuses to flash anything that wasn't built for `KNOB_ENV`.

### Prerequisites

1. **Docker Desktop**, running. `docker --version` and `docker info` must both work.
2. **Python 3.12** on PATH. Check: `python --version`
3. A USB-C **data** cable (charge-only cables show no COM port).
4. A 2.4 GHz WiFi network for the device. The ESP32-S3 has no 5 GHz radio.

### One-time setup

1. Create a Python environment for esptool. Use `python -m`, not `pip.exe`, which is blocked on managed laptops:

   ```
   python -m venv .venv
   .venv\Scripts\python.exe -m pip install esptool
   ```

2. Edit `settings.cmd`: set `PORT` to your COM port (Device Manager > Ports), and check `KNOB_ENV`.

3. **The board has two MCUs (ESP32-S3 and ESP32-U4) behind one USB-C port, and the plug orientation selects which one you reach.** Confirm you are talking to the S3 (read-only, writes nothing):

   ```
   .venv\Scripts\python.exe -m esptool --chip esp32s3 --port COM3 flash-id
   ```

   Expect "ESP32-S3" and a 16 MB flash. If the port isn't found, or esptool reports a different chip, unplug, flip the plug 180 degrees and try again. Only ever flash the S3.

4. **Back up the factory firmware before the first flash** (the S3's 16 MB, about a minute):

   ```
   .venv\Scripts\python.exe -m esptool --chip esp32s3 --port COM3 read-flash 0 0x1000000 factory_backup_s3.bin
   ```

   Restore with `write-flash 0x0 factory_backup_s3.bin`. `*.bin` is git-ignored; keep a copy outside the project folder too. Waveshare also publishes the factory BIN files (see the wiki's Resources section), which restore both chips.

### Build, merge, flash

With Docker running, from PowerShell in the project folder, in order:

```
.\build.cmd
.\merge.cmd
.\deploy.cmd
```

Or run all three with `.\full_deploy.cmd`; it stops at the first step that fails. Each script also stops with a message if the step before it failed.

- **`build.cmd`** compiles in a container. The first run is slow: it builds a small image from `Dockerfile` (Python, git, PlatformIO, esptool), then downloads the platform, toolchain and libraries. The toolchain lives in the `pio_cache_knob` Docker volume; libraries and build output live in `pio_work_knob`, because scanning thousands of library files across the Windows bind mount makes builds very slow. Only the three images needed for merging are copied back to `.pio/build/<env>/`. Check the output ends in `BUILD OK`. Force a clean rebuild with `docker volume rm pio_work_knob`.
- **`merge.cmd`** merges bootloader, partition table, `boot_app0` and the app into one `merged.bin` flashed at `0x0`, and records which env it came from.
- **`deploy.cmd`** refuses to flash if `merged.bin` is older than `firmware.bin` or wasn't built for `KNOB_ENV`, then flashes the S3. **This overwrites everything on it, including the saved WiFi credentials and settings** (location, theme, units), so you redo the WiFi setup afterwards. Close any serial monitor first.

#### Keeping WiFi and settings: `--keep-wifi`

The full image also rewrites the settings area. For everyday iteration, flash only the app instead:

```
.\build.cmd
.\deploy.cmd --keep-wifi
```

or in one go, `.\full_deploy.cmd --keep-wifi` (build, then flash the app; there is no merge step). It writes just `firmware.bin` at `0x10000` (the app0 slot), so the WiFi credentials, location and other saved settings survive. Both scripts also take an env name (`.\deploy.cmd --keep-wifi esp32-s3-knob-18-bringup`). `build.cmd` accepts the flag but ignores it with a note: compiling cannot touch settings, only flashing can.

It carries the same wrong-board guard (only the knob envs), and `build.cmd` deletes the previous `firmware.bin` before building, so a failed build leaves nothing to flash by mistake. Two limits: the board must already carry a full image of this layout, so **do a plain `deploy.cmd` the first time**, and **again after any browser OTA update** (an app-only flash does not reset which OTA slot the bootloader picks).

If flashing can't connect: flip the USB-C plug 180 degrees. Still failing: hold the S3's BOOT button, power the board on again, release, and run `deploy.cmd` again. After a successful flash, power-cycle the board; on first boot join the `CapsuleRadar-Setup` WiFi as usual.

The scripts are plain `.cmd` files, so PowerShell execution policy doesn't affect them (`deploy.cmd` uses one inline `powershell -Command` for the timestamp check, which isn't a script file).

> The most common mistake is flashing a stale `merged.bin` because the build or merge was skipped or failed. `deploy.cmd` guards against the obvious case, but still check the build ended in `BUILD OK`.

### Assumptions to verify on the first real knob build

- `scripts/docker_merge.sh` keeps the flash mode, frequency and size from the bootloader the build produced (DIO, 80 MHz, 16 MB), even though `platformio.ini` says `flash_mode = qio`: PlatformIO's espressif32 builder maps `qio` to `dio` for the image header and uploads with `--flash_mode dio`, and upstream's `scripts/build_webflasher.sh` merges with `dio` too. Checked on the 1.75 build: `merged.bin` header reads DIO / 80m / 16MB.
- It also assumes the `default_16MB.csv` layout (otadata `0xe000`, app `0x10000`). A different partition file needs different offsets.
- `boot_app0.bin` is located by searching the PlatformIO package cache rather than hardcoded. Verified against the pioarduino platform: it resolves to `/root/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin`.
- The upstream flags assume native USB CDC for serial. How the knob routes USB to the S3 (UART bridge or native) is unconfirmed; check the schematic before relying on serial output.

### Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `docker: error during connect ... dockerDesktopLinuxEngine` | Docker Desktop isn't running. Start it, wait for the engine, check `docker info` |
| Build fails fetching a library from GitHub | The build image needs `git`; the `Dockerfile` installs it. If you made your own image, add git |
| Image build fails: apt "Clearsigned file isn't valid, got 'NOSPLIT'" or "does the network require authentication?" | The network intercepts plain HTTP (port 80). The `Dockerfile` switches apt to HTTPS for this reason; keep that line |
| Build tries to compile every env / the SDL simulator | Something ran `pio run` without `-e`. Always go through `build.cmd` |
| Build is slow at "Scanning dependencies" | Libraries must live in the `pio_work_knob` volume, not the Windows folder (`build.cmd` does this) |
| `deploy.cmd`: "Refusing to flash" | `merged.bin` was built for a different env than `KNOB_ENV`. Rebuild and re-merge the knob env |
| esptool "Failed to connect", or no COM port | Flip the USB-C plug, close serial monitors, or use the BOOT method above |
| esptool says the chip isn't ESP32-S3 | You're connected to the ESP32-U4. Flip the plug |
| `pip.exe` / `gcc.exe` "Access is denied" | Managed-laptop policy; this Docker workflow is the way around it |

## Flash from your browser (no toolchain)

Makers can flash without installing anything using **ESP Web Tools** (Chrome or Edge on desktop):

1. Open the **[web flasher](https://socquique.github.io/capsule-radar/)** (the project's GitHub Pages site).
2. **Pick your board** (1.75″ or 1.43″) — the wrong image boots with a black screen.
3. Plug the board in with a USB-C **data** cable and click **Install**.

The flasher is built and published automatically by GitHub Actions ([`.github/workflows/webflasher.yml`](.github/workflows/webflasher.yml)) on every push to `main` — enable it once in **Settings → Pages → Source = GitHub Actions**. Tagged releases (`git tag v1.0.0 && git push origin v1.0.0`) also attach a ready-to-flash `CapsuleRadar-esp32s3.bin` to a **GitHub Release** via [`release.yml`](.github/workflows/release.yml). To preview the flasher locally:

```bash
./scripts/build_webflasher.sh                      # build + merge into web/flash/
python3 -m http.server -d web/flash 8000           # serve (Web Serial works on localhost)
# open http://localhost:8000
```

## Desktop simulator

The whole UI is portable LVGL and runs on your computer (SDL2) — great for iterating without hardware:
```bash
pio run -e native -t exec     # opens a 466×466 window (needs SDL2: `brew install sdl2`)
```
Mouse = touch · `T` = switch theme · close the window to quit.

## Configuration

Browse to `http://capsuleradar.local/` (or the device IP) on the same WiFi to set the **center lat/lon**, **display range**, **theme** and **brightness**, or to **reset WiFi**. Saving restarts the device to apply.

## Repo layout

```
src/
  config.h           pins + tunables (Dénia, Spain by default)
  main.cpp           tasks, WiFi/NTP, web config, brightness/IMU glue
  display.*          CO5300 (Arduino_GFX) + LVGL bring-up
  radar_view.*       the radar scope, aircraft, themes
  ui.*               views (radar/list/stats) + detail card + HUD
  touch_cst9217.*    capacitive touch driver
  imu_qmi8658.*      accelerometer (face-down sleep)
  battery.*          AXP2101 battery gauge
  rtc_pcf85063.*     PCF85063 real-time clock
  adsb_client.*      airplanes.live / adsb.fi / adsb.lol fetch + parse (with adsb_pacing.h)
  route*.* route.*   origin→destination lookup (adsbdb)
  sim_main.cpp       native SDL simulator (not flashed)
include/lv_conf.h    LVGL config (v8)
web/flash/           browser web-flasher (ESP Web Tools) for makers
scripts/             build_webflasher.sh (merge firmware -> single .bin)
docs/                hardware / data-source / architecture notes
```

## Community ports & forks

The reference board is the **Waveshare ESP32-S3-Touch-AMOLED-1.75**, but other boards are welcome upstream: a port contributed as a pull request (its own PlatformIO env + board header in [`src/boards/`](src/boards/)) gets built by the CI on every release alongside the 1.75 — the in-tree **1.43** port is the worked example to copy, and the **2.1"** port by @alexzogh is now in-tree too (experimental, above). Ports that prefer to stay independent live as forks, linked here (MIT: fork away, and tell me so I can add yours).

- **[Capsule Radar for the Waveshare ESP32-S3-Touch-LCD-2.1](https://github.com/alexzogh/capsule-radar/tree/port/esp32-s3-lcd-21)** by **@alexzogh (STLWarehouse)** — a full port to the 2.1" round LCD (ST7701), plus new features: **double-tap to track an aircraft** (scope re-centres on it), a **clock face on idle**, and the busy-airspace **query-radius fix** now merged back into this firmware. The board itself is now built in-tree with every release (files with `lcd21` in the name); the fork keeps its extra features.
- **[Capsule Radar for the Waveshare 2.8" (non-touch)](https://github.com/ijord/capsule-radar)** by **@ijord** — port to the 2.8" ST7701 panel, with a smoother sweep compositor and the **PNG heap-corruption fix** that shipped upstream in v1.3.28. Thank you!
- **[Skyglass](https://github.com/SilentWolf75/skyglass)** by **@SilentWolf75** — an independently evolved sibling (190+ commits): **shaded OpenStreetMap basemap rendered on-device** and **ESP32-P4** support.

## Data & license

**Firmware / code: [MIT](LICENSE)** — fork and build on it freely (keep the notice). Aircraft data: **airplanes.live** (free, **non-commercial / educational** — exactly this project; be polite with request cadence). Routes: **adsbdb.com** (free). Personal/hobby project. The 3D-printed enclosure is published on [MakerWorld](https://makerworld.com/en/models/2907695-capsule-radar-live-flight-radar-desk-gadget) (enclosure + this firmware).
