#!/usr/bin/env bash
# Runs INSIDE the build container, called by merge.cmd. Not meant to be run directly on Windows.
# Merges bootloader + partition table + boot_app0 + app into one image flashed at 0x0.
#
# Flash mode/freq/size are KEPT from the bootloader the build produced (--flash-* keep).
# That header is DIO / 80m / 16MB even though platformio.ini says flash_mode = qio:
# PlatformIO's espressif32 builder deliberately maps qio -> dio for the image header and
# uploads with --flash_mode dio. Forcing qio here would differ from a normal "pio run -t upload".
# The vendor demo for the knob is also built DIO at 80 MHz.
#
# Assumption, to be checked on the first real knob build (see README):
#  - Offsets 0x0 / 0x8000 / 0xe000 / 0x10000 match Arduino's default_16MB.csv layout
#    (same as partitions_16MB.csv in this repo). A different partition file needs new offsets.
set -euo pipefail

ENV_NAME="${1:?usage: docker_merge.sh <platformio env>}"
OUT=".pio/build/$ENV_NAME"

# Locate boot_app0.bin instead of hardcoding the framework package directory name.
BOOT_APP0="$(find /root/.platformio/packages -path '*tools/partitions/boot_app0.bin' -print -quit)"
[ -n "$BOOT_APP0" ] || { echo "boot_app0.bin not found under /root/.platformio/packages" >&2; exit 1; }
echo "boot_app0: $BOOT_APP0"

rm -f merged.bin
python -m esptool --chip esp32s3 merge-bin -o merged.bin \
  --flash-mode keep --flash-freq keep --flash-size keep \
  0x0 "$OUT/bootloader.bin" \
  0x8000 "$OUT/partitions.bin" \
  0xe000 "$BOOT_APP0" \
  0x10000 "$OUT/firmware.bin"

[ -s merged.bin ] || { echo "merged.bin was not created" >&2; exit 1; }
echo "merged.bin created"
