# Capsule Radar Knob

A live **ADS-B aircraft radar** for the **Waveshare ESP32-S3-Knob-Touch-LCD-1.8**: a 1.8-inch round touchscreen with a rotary knob. It pulls nearby aircraft from free online feeds over WiFi and plots them on a radar scope centred on your location. Turn the knob to step through aircraft and airports, swipe between pages, and read the details on a card.

> **This repository is a homage to, and a derivative of, [Capsule Radar](https://github.com/socquique/capsule-radar) by Quique Tortosa ([@socquique](https://github.com/socquique)).** The idea, the radar scope, the feed client, the web configuration, the WiFi setup portal, the sweep compositor and the idle clock are his work; this fork only adapts them to a different device and adds to them. If you own one of the round AMOLED boards that his project targets, use **his** project, not this one. See [Credits](#credits-and-licence).

It is deliberately **specific to one device**, the knob board. The pin map, panel, touch controller, haptics and every layout in this repo are tuned for its round 360 x 360 screen. It is not a general port, and it is not a replacement for the original.

## Contents

- [Credits and licence](#credits-and-licence)
- [What is different from the original](#what-is-different-from-the-original)
- [Hardware](#hardware)
- [Using the device](#using-the-device)
- [Deploying: build, flash, update](#deploying-build-flash-update)
- [Regenerating the data tables](#regenerating-the-data-tables)
- [Tuning reference](#tuning-reference)
- [Repo layout](#repo-layout)
- [Troubleshooting](#troubleshooting)

## Credits and licence

**The original project.** Capsule Radar (formerly "Plane Radar 2.0") by **Quique Tortosa (@socquique)**: <https://github.com/socquique/capsule-radar>. It runs on the Waveshare ESP32-S3-Touch-AMOLED-1.75 and 1.43 boards and comes with a printable case on [MakerWorld](https://makerworld.com/en/models/2907695-capsule-radar-live-flight-radar-desk-gadget) and a browser flasher. Almost everything that makes this work is his: the streaming ADS-B client with per-provider pacing, the radar scope and its themes, route lookup, the configuration web page and WiFi portal, the QSPI sweep compositor, the idle clock, and the overall structure of the code.

**How this repo relates to it.** It started as a fork of that project at upstream commit `ecf2110` ("FW 1.6.0: idle clock, LCD 2.1 wake-only touch, 429 pacing", 2026-10-27) and keeps the original git history. Everything after that point is this fork's work. Bugs in the fork are the fork's, so please report them here and not upstream.

**Licence.** MIT, copyright (c) 2026 Quique Tortosa (socquique), exactly as in [`LICENSE`](LICENSE). The notice stays with every copy; this fork's additions are offered under the same licence.

**Data and third-party material.**

| What | Source | Notes |
|---|---|---|
| Live aircraft | [airplanes.live](https://airplanes.live), [adsb.fi](https://adsb.fi), [adsb.lol](https://adsb.lol) | Free, non-commercial. Keep the polling polite. See [`docs/DATA_SOURCE.md`](docs/DATA_SOURCE.md) |
| Routes | [adsbdb.com](https://www.adsbdb.com) | Origin to destination, cached on the device |
| Weather radar | [RainViewer](https://www.rainviewer.com) | Rain radar frames |
| Satellite clouds | [EUMETSAT EUMETView](https://view.eumetsat.int) (Meteosat) | Tested over Europe, where it works. Over Australia the service returned a black frame (no data there) |
| Forecast | [Open-Meteo](https://open-meteo.com) | 3-day forecast |
| Coastlines | [Natural Earth](https://www.naturalearthdata.com), public domain | Embedded as a table (`src/coastline_data.h`) |
| Airports | [OurAirports](https://ourairports.com/data/), public domain | Embedded (`src/airports_data.h`, `src/airports_info.h`) |
| Time zones | IANA time zone database | Baked into the airport table at generation time |
| Panel start-up table | Waveshare's demo code | The values are **Waveshare's**. Their demo ships no licence file, so `src/boards/st77916_knob_init.h` is **not covered by this project's MIT licence**. It is committed only so the project builds without the demo, and can be regenerated with `bringup/gen_init.py` |

This is an independent fork and is not affiliated with Waveshare.

## What is different from the original

The original targets 466 x 466 AMOLED boards with a PMIC, audio codec and IMU. This fork targets a 360 x 360 round LCD with a knob, and is built around that.

- **The knob is the main control.** It steps through the radar's aircraft and airports clockwise by where they sit on the screen (with a "none" stop), scrolls the aircraft list one row per click, and flips the weather page between modes. Every step gives a haptic tick from the DRV2605L.
- **Four pages**: Radar, aircraft List, Settings and Weather. The device always opens on the radar.
- **Airports as first-class objects.** Bright magenta crosses on the scope, selectable by tap or knob, with an info card (IATA and ICAO codes, name, **local time using the airport's own time zone rules**, distance, bearing, elevation, country), and rows in the list with a building icon.
- **A list that is a wheel**, with plane, jet, helicopter and building icons; the centre row is the selected item.
- **The sweep makes things glow.** Aircraft and airports light up bright green as the radar line passes, rising over 300 ms and fading over 3 s.
- **Smooth page changes.** A swipe slides a pre-rendered picture of the next page at about 40 frames a second, instead of redrawing the whole screen on every frame of a finger drag (which managed about 12).
- **A Settings page** with a range dropdown, a "Military only" checkbox, WiFi signal strength and the config page address.
- **Less clutter when it is busy.** With something selected, the other aircraft hide their flight code and altitude, once at least a chosen number of aircraft are on screen (a setting on the config page).
- **Weather sized for the glass**: radar and cloud pictures at a 320 px circle, the satellite picture covering a true 75 km radius, a location line, and a 3-day forecast as the default.
- **A build workflow for locked-down Windows machines**: everything compiles inside Docker, and only flashing runs on Windows. See [Deploying](#deploying-build-flash-update).
- Compiled with `-O2` instead of the framework's default size optimisation. LVGL's software drawing is a large part of every frame, so this helps a little on some pages (a few milliseconds a frame); it is not a big win on its own.

## Hardware

**Waveshare ESP32-S3-Knob-Touch-LCD-1.8** ([wiki](https://www.waveshare.com/wiki/ESP32-S3-Knob-Touch-LCD-1.8)).

| | |
|---|---|
| MCU | **ESP32-S3R8** (8 MB octal PSRAM, 16 MB flash). The board also carries a **second MCU, an ESP32-U4WDH**, behind the same USB-C port |
| Display | 1.8" round, 360 x 360, **ST77916** over QSPI. Backlight on GPIO 47 |
| Touch | **CST816D**, I2C address `0x15` (SDA GPIO 11, SCL GPIO 12) |
| Rotary knob | Two-pin encoder: GPIO 8 (A, clockwise = +1) and GPIO 7 (B), read by a 3 ms timer |
| Haptics | **DRV2605L**, I2C address `0x5A`, tick effect 5 |
| Battery | Sense on GPIO 1 (ADC behind a divide-by-two); shown only while USB is not connected |
| Not used | GPS (the board has none), the DAC/audio, the TF card slot |

All pins live in [`src/boards/board_knob_18.h`](src/boards/board_knob_18.h).

> **The USB-C plug orientation selects which of the two MCUs you reach.** Flip the plug and you are talking to the other chip, usually on a different COM port. **Only ever flash the ESP32-S3.** `deploy.cmd` and esptool both tell you which chip they found; if it is not the S3, flip the plug.

## Using the device

**First boot.** Join the open WiFi network **`CapsuleRadar-Setup`** from your phone, pick your 2.4 GHz network (the S3 has no 5 GHz radio) and enter the password. Then browse to **`http://capsuleradar.local/`** (or the IP shown on the Settings page) to set your centre point, units, theme and the rest.

**Pages.** Swipe left and right: Radar, List, Settings, Weather.

| Page | Touch | Knob |
|---|---|---|
| **Radar** | Tap an aircraft or airport to select it and show its card. Tap empty background: hides the card but keeps the selection; tap it again with no card showing to clear the selection | Steps the selection clockwise by position, through a "none" stop. The card returns after the knob has been still for 1.5 s |
| **List** | The centre row is the selected item. Aircraft first (nearest first), then airports in range | One row per click |
| **Settings** | **Range** dropdown (10, 20, 30, 50, 100, 150 km), **Theme** dropdown (Phosphor, Orb, Amber CRT, Military), **Military only** checkbox, WiFi signal bars and the config page address. The firmware version is not shown here: it is in the serial boot log and the config page footer | Not used |
| **Weather** | Tap to change mode | Changes mode: WX RADAR, SAT CLOUDS, 3-DAY FORECAST |

**Idle.** After 60 s with no touch or knob the radar turns into an analog clock (changeable on the config page, "Clock or dim after"). The first touch or click only wakes it.

**Military only** is the same setting as the config page's. Switching it on filters what is already on screen at once. Switching it off has to fetch the other aircraft, so they appear within a few seconds. With it on and none in range, the scope still shows its map and airports, and the List says "No aircraft or airports in range".

**Airports** appear only when they are inside the current range, and only with "Show airports" on (the config page).

## Deploying: build, flash, update

Everything is driven by five scripts in the project root. The build and merge run **inside Docker**; flashing runs **on Windows** through `esptool` in a small Python venv. This works on managed laptops that block PlatformIO's compiler, `pip.exe` and similar ("Access is denied"): the container is not subject to that policy.

| Script | What it does |
|---|---|
| `settings.cmd` | Shared settings: the COM `PORT` (default `COM4`), the env names, the Docker image and volume names. Edit here only |
| `build.cmd [env]` | Compiles in Docker. Deletes the previous `firmware.bin` first, so a failed build cannot be flashed by accident |
| `merge.cmd [env]` | Merges bootloader, partition table, `boot_app0` and the app into one `merged.bin` for address `0x0` |
| `deploy.cmd [--keep-wifi] [env]` | Flashes. Checks first that the image is for the knob and not stale |
| `full_deploy.cmd [--keep-wifi] [env]` | Runs the steps above in order and stops at the first failure |

The default env is `esp32-s3-knob-18` (the radar firmware). The other env, `esp32-s3-knob-18-bringup`, is a standalone hardware test, see [bring-up](#bring-up-the-hardware-test).

### Prerequisites

1. **Windows**, **PowerShell** and **git**.
2. **Docker Desktop**, **running** (`docker info` must work). The first build needs internet to download the image, the PlatformIO platform and the libraries.
3. **Python 3.12** on PATH (`python --version`).
4. A USB-C **data** cable (a charge-only cable shows no COM port).
5. **Waveshare's demo code**, only if you want to regenerate the display table or build the bring-up firmware (from the Resources section of the [wiki page](https://www.waveshare.com/wiki/ESP32-S3-Knob-Touch-LCD-1.8)). The radar firmware builds without it.

### One-time setup on a new machine

Run these from PowerShell.

1. **Get the code.**

   ```
   git clone https://github.com/nickraphael/capsule-radar-knob.git
   cd capsule-radar-knob
   ```

2. **Create the flashing environment** (esptool, pure Python). Use `python -m`, not `pip.exe`, which managed laptops block:

   ```
   python -m venv .venv
   .venv\Scripts\python.exe -m pip install esptool
   ```

3. **(Optional) Regenerate the display start-up table.** It is already in the repo (`src/boards/st77916_knob_init.h`: Waveshare's panel register values, kept so a fresh clone builds without their demo; see the [credits](#credits-and-licence) for the licence note), so you can skip this for the radar firmware. You only need the demo to refresh that file, or to make the copy the bring-up firmware needs. Point the generator at `lcd_bsp.c` inside the demo:

   ```
   python bringup/gen_init.py "<demo folder>\Arduino\examples\08_LVGL_Test\lcd_bsp.c" src/boards/st77916_knob_init.h
   ```

   For the bring-up test firmware, run it with no output path, which writes that firmware's own copy, `bringup/st77916_knob_init.h` (this copy is git-ignored):

   ```
   python bringup/gen_init.py "<demo folder>\Arduino\examples\08_LVGL_Test\lcd_bsp.c"
   ```

4. **Set your COM port** in `settings.cmd` (Device Manager, Ports; the knob appears as "USB Serial Device"). To override it for one session without editing the file: `$env:PORT = "COM5"`.

5. **Confirm you are talking to the ESP32-S3.** This only reads; it writes nothing:

   ```
   .venv\Scripts\python.exe -m esptool --chip esp32s3 --port COM4 flash-id
   ```

   Expect "ESP32-S3" and a 16 MB flash. If the port is missing or it reports another chip, flip the USB-C plug 180 degrees and retry.

6. **Back up the factory firmware** before the first flash (about a minute):

   ```
   .venv\Scripts\python.exe -m esptool --chip esp32s3 --port COM4 read-flash 0 0x1000000 factory_backup_s3.bin
   ```

   Keep a copy outside the project too (`*.bin` is git-ignored). To restore it:

   ```
   .venv\Scripts\python.exe -m esptool --chip esp32s3 --port COM4 write-flash 0x0 factory_backup_s3.bin
   ```

### First flash

A first flash writes the whole chip. **This erases the saved WiFi and settings.**

```
.\build.cmd
.\merge.cmd
.\deploy.cmd
```

or all three at once:

```
.\full_deploy.cmd
```

Check that the build ends in **`BUILD OK`**. The first build is slow (several minutes) while Docker builds the image and downloads the toolchain. Later builds reuse the caches. Close any serial monitor first, then power-cycle the board after the flash and do the [first-boot setup](#using-the-device).

### Everyday update (keeps your WiFi and settings)

```
.\build.cmd
.\deploy.cmd --keep-wifi
```

or `.\full_deploy.cmd --keep-wifi` for both. This flashes **only the app** (`firmware.bin` at `0x10000`), so the WiFi credentials, location and other saved settings survive; there is no merge step. Two limits:

- The board must already carry a full image of this layout, so do the [first flash](#first-flash) once.
- Do a plain `deploy.cmd` **again after any browser OTA update**, because an app-only flash does not reset which OTA slot the bootloader picks.

### Updating over WiFi instead of USB

Browse to `http://capsuleradar.local/update` and upload **`.pio\build\esp32-s3-knob-18\firmware.bin`** (the app image, not `merged.bin`). The page's own wording mentions the upstream project's release file; ignore that.

### What the Docker setup is

- **Image `capsule-knob-build`**: Python 3.12, git, PlatformIO and esptool, built once from [`Dockerfile`](Dockerfile) by `build.cmd`. apt is switched to HTTPS because some networks tamper with plain HTTP.
- **Volume `pio_cache_knob`**: the PlatformIO toolchain and platform (`/root/.platformio`).
- **Volume `pio_work_knob`**: libraries and build output (`/pio_work`). They live in a volume, not the Windows folder, because scanning thousands of library files across the Windows mount makes builds very slow. Only `firmware.bin`, `bootloader.bin` and `partitions.bin` are copied back to `.pio\build\<env>\`.
- Neither volume is shared with other projects, so a clean rebuild here never wipes theirs.

To reset:

```
docker volume rm pio_work_knob      # forces a clean rebuild of the libraries and the project
docker volume rm pio_cache_knob     # also re-downloads the toolchain
docker rmi capsule-knob-build       # rebuilds the image (picks up newer PlatformIO / esptool)
```

### Checking you have not broken the shared code

The AMOLED envs are still in `platformio.ini`, and much of `src/` is shared with them. This fork does not support those boards and has not tested them on hardware, but it keeps them compiling as a regression check:

```
.\build.cmd esp32-s3-amoled-175
.\build.cmd esp32-s3-amoled-143
```

`deploy.cmd` refuses to flash anything not built for the knob. The `esp32-s3-lcd-21` env is slow to build and is not part of the routine check. The `native` desktop-simulator env is inherited and has not been maintained by this fork.

### Reading the serial log

`pyserial` is installed with esptool. Opening a port can reset some boards, so switch DTR and RTS off:

```
.venv\Scripts\python.exe -m serial.tools.miniterm --dtr 0 --rts 0 COM4 115200
```

Quit with Ctrl+]. The log shows what hardware answered at start-up (`[display]`, `[touch]`, `[knob]`, `[haptic]`), then the feed, weather and clock activity.

### Bring-up (the hardware test)

`esp32-s3-knob-18-bringup` builds `bringup/main.cpp`, a standalone program with no radar code that tests the panel, touch, knob, haptics and memory settings. It needs `bringup/st77916_knob_init.h` (setup step 3). It goes through the same scripts:

```
.\build.cmd esp32-s3-knob-18-bringup
.\merge.cmd esp32-s3-knob-18-bringup
.\deploy.cmd
```

## Regenerating the data tables

Both tables are generated, committed, and normally never touched. Regenerate them only to refresh the data.

**Coastlines** (`src/coastline_data.h`), from the Natural Earth 1:10m coastline GeoJSON:

```
python tools/gen_coastline.py ne_10m_coastline.geojson src/coastline_data.h
```

**Airports** (`src/airports_data.h` and `src/airports_info.h`), from OurAirports. The two files are generated together because index *i* must mean the same airport in both:

```
python tools/gen_airports.py airports.csv src/airports_data.h src/airports_info.h
```

Download `airports.csv` from <https://davidmegginson.github.io/ourairports-data/airports.csv>. Without the third argument only `airports_data.h` is written. The third argument also needs the time zone lookup, installed into a throwaway venv, not the flashing one:

```
python -m venv tzvenv
tzvenv\Scripts\python.exe -m pip install --only-binary :all: "timezonefinder<6.2" "h3<4" numpy tzdata
```

(The pins matter: newer `timezonefinder` needs a C compiler to install, and the older one needs the older `h3` API.) On a managed laptop, create that venv in an ordinary folder rather than `%TEMP%`: policy there can refuse to load compiled libraries from the temp folder. The airports' local-time rules are fixed at generation time, so regenerate now and then, since countries change their clocks.

## Tuning reference

Behaviour is set by macros in [`src/boards/board_knob_18.h`](src/boards/board_knob_18.h), each commented in the file. Change one, rebuild and flash.

| Macro | Value | What it does |
|---|---|---|
| `BOOT_VIEW` | `0` | Page shown at start (0 radar, 1 list, 2 settings, 3 weather) |
| `IDLE_DIM_MS` | `60000` | Idle time before the clock; a value saved from the config page wins |
| `KNOB_CARD_DELAY_MS` | `1500` | How long the knob must be still before the info card returns |
| `SWIPE_SLIDE`, `SWIPE_SLIDE_MS` | `1`, `280` | The pre-rendered page slide, and its duration. Delete `SWIPE_SLIDE` to go back to a plain finger drag |
| `TOUCH_SCROLL_LIMIT` | `16` | How far (px) a finger may drift and still count as a tap (LVGL's default is 10) |
| `AIRPORT_SELECT` | `1` | Airport crosses, selection, info card and list rows |
| `SWEEP_GLOW`, `GLOW_RISE_MS`, `GLOW_FADE_MS`, `GLOW_LEVELS` | `1`, `300`, `3000`, `16` | The glow as the sweep passes |
| `LABELS_SELECTED_ONLY` | `1` | Other aircraft hide their text while one is selected (threshold on the config page) |
| `SETTINGS_PAGE`, `RANGE_ON_STATS`, `WIFI_BARS_ON_STATS` | `1` | The Settings page, with the range dropdown and the WiFi bars on it |
| `WX_VIEW_D` | `320` | Diameter (px) of the weather pictures; at most 360 |
| `CLOUD_RADIUS_KM` | `75` | Radius the satellite picture covers |
| `STATS_*_Y` | | Vertical positions of the Settings page's rows |

The zoom steps are `RANGE_STEPS_KM` in [`src/config.h`](src/config.h) (10, 20, 30, 50, 100, 150 for the knob). Everything else (centre point, theme, units, max aircraft, show airports, military only, the aircraft-text threshold, WiFi reset) is on the config page at `http://capsuleradar.local/` and is saved on the device.

## Repo layout

```
src/
  main.cpp               tasks, WiFi and NTP, the web config page, the knob loop
  boards/board_knob_18.h pins, panel and every knob-specific setting (see the table above)
  display.*              ST77916 (Arduino_GFX) + LVGL, the sweep compositor, the page slide
  radar_view.*           the scope: aircraft, airports, selection, themes, the glow
  ui.*                   the four pages, the info cards, the list wheel, the Settings controls
  knob.*  haptic.*       rotary knob decoder and the DRV2605L
  touch_cst816d.cpp      touch controller driver
  airports*.*  posix_tz.h  airport markers and details, and local time for any time zone
  adsb_client.*  adsb_pacing.h   feed fetch and parse (inherited)
  route*.*  photo*.*     route and aircraft photo lookup (inherited)
  weather*.*  wx_radar*.*  cloud_image*.*   forecast, rain radar and satellite pictures
  coastline*.*           coastline overlay
bringup/                 standalone hardware test, and gen_init.py (the display table generator)
tools/                   gen_airports.py, gen_coastline.py
scripts/                 docker_build.sh, docker_merge.sh (run inside the container)
tests/                   host-side unit tests, inherited from upstream (not re-run by this fork)
docs/                    the original project's notes, written for the AMOLED boards
build.cmd  merge.cmd  deploy.cmd  full_deploy.cmd  settings.cmd  Dockerfile
include/lv_conf.h        LVGL configuration (v8)
```

`.github/workflows/` and `web/flash/` are inherited from the original project (its AMOLED browser flasher and release builds). They are not used for the knob.

## Troubleshooting

| Symptom | Cause and fix |
|---|---|
| Bring-up build fails with a missing `st77916_knob_init.h` | The bring-up firmware's own copy is git-ignored and was never generated here: run [setup step 3](#one-time-setup-on-a-new-machine) with no output path. (The radar firmware does not need this; its copy is in the repo) |
| `docker: error during connect ... dockerDesktopLinuxEngine` | Docker Desktop is not running. Start it and wait for `docker info` to work |
| Image build fails: apt "Clearsigned file isn't valid" | The network intercepts plain HTTP. The `Dockerfile` already switches apt to HTTPS; keep that line |
| Build tries to compile every env, including the SDL simulator | Something ran `pio run` without `-e`. Always go through `build.cmd` |
| Build is very slow at "Scanning dependencies" | Libraries must live in the `pio_work_knob` volume, not the Windows folder (`build.cmd` does this) |
| `deploy.cmd`: "Refusing to flash" | `merged.bin` was built for another env, or is older than `firmware.bin`. Rebuild and re-merge |
| esptool "Failed to connect", or no COM port | Close serial monitors. Flip the USB-C plug 180 degrees. Still failing: hold the S3's BOOT button, power the board on again, release, run again |
| esptool says the chip is not an ESP32-S3 | You are connected to the ESP32-U4. Flip the plug |
| `--keep-wifi` flash boots the wrong image | You flashed after a browser update without a full flash since: do a plain `deploy.cmd` |
| `pip.exe` / `gcc.exe` "Access is denied" | Managed-laptop policy. This Docker workflow is the way around it |
| Satellite picture is black | EUMETSAT has no data for your area (for example Australia). Rain radar and forecast still work |
| The screen stays black after flashing | Check the boot log (above) first; then power-cycle. Make sure you flashed the S3, not the U4 |
