@echo off
rem Shared settings for build.cmd, merge.cmd, deploy.cmd and full_deploy.cmd. Edit here only.

rem PlatformIO env that targets the Waveshare ESP32-S3-Knob-Touch-LCD-1.8.
rem NOTE: this env does not exist in platformio.ini yet. Add it with the port, then make the name match.
set "KNOB_ENV=esp32-s3-knob-18"
rem Bring-up test firmware (bringup\main.cpp), built and flashed through the same scripts.
set "KNOB_BRINGUP_ENV=esp32-s3-knob-18-bringup"

rem Docker image and volumes. Kept separate from other projects (for example WLED_CONTROLLER)
rem so a clean rebuild here never wipes their caches.
set "IMAGE=capsule-knob-build"
set "VOL_CACHE=pio_cache_knob"
set "VOL_WORK=pio_work_knob"

rem Flashing runs on Windows (not in Docker) through the venv Python.
set "PYTHON=.venv\Scripts\python.exe"
rem COM port of the S3 (plug orientation selects the chip and changes the port; COM4 was the S3).
rem Override for one run with:  set PORT=COMx
if not defined PORT set "PORT=COM4"
