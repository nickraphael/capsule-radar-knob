#!/usr/bin/env bash
# Runs INSIDE the build container, called by build.cmd. Not meant to be run directly on Windows.
# PLATFORMIO_WORKSPACE_DIR=/pio_work (set by build.cmd) keeps libraries and build output in a
# Docker volume: scanning thousands of library files across the Windows bind mount is very slow.
# Only the three images needed for merging are copied back into the project's .pio folder.
set -euo pipefail

ENV_NAME="${1:?usage: docker_build.sh <platformio env>}"

# Always build one explicit env. platformio.ini has no default_envs, so a bare "pio run"
# would try to build every env, including the SDL2 desktop simulator.
pio run -e "$ENV_NAME"

mkdir -p ".pio/build/$ENV_NAME"
for f in firmware.bin bootloader.bin partitions.bin; do
  cp "/pio_work/build/$ENV_NAME/$f" ".pio/build/$ENV_NAME/$f"
done
echo "Copied firmware.bin, bootloader.bin, partitions.bin to .pio/build/$ENV_NAME/"
