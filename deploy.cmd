@echo off
setlocal
cd /d "%~dp0"
call settings.cmd

rem Arguments: --keep-wifi = flash ONLY the app image, leaving the settings area (WiFi, location,
rem theme...) untouched; an optional env name then says which build's firmware.bin to flash.
set "KEEP_WIFI=0"
set "ENV_ARG="
:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="--keep-wifi" (
  set "KEEP_WIFI=1"
) else (
  set "ENV_ARG=%~1"
)
shift
goto :parse_args
:args_done

if "%KEEP_WIFI%"=="1" goto :app_only

echo === DEPLOY: full image ===
if not "%ENV_ARG%"=="" echo Note: an env name only applies with --keep-wifi. Ignored: the image's env comes from merge.cmd.
if not exist merged.bin (
  echo No merged.bin. Run build.cmd then merge.cmd first.
  exit /b 1
)
if not exist ".pio\merged_env.txt" (
  echo No env marker for merged.bin. Run merge.cmd first.
  exit /b 1
)
set /p MERGED_ENV=<".pio\merged_env.txt"
rem Only images built for the knob may be flashed: the radar env or the bring-up env.
set "ENV_OK=0"
if /i "%MERGED_ENV%"=="%KNOB_ENV%" set "ENV_OK=1"
if /i "%MERGED_ENV%"=="%KNOB_BRINGUP_ENV%" set "ENV_OK=1"
if "%ENV_OK%"=="0" (
  echo merged.bin was built for %MERGED_ENV%, not for %KNOB_ENV% or %KNOB_BRINGUP_ENV%. Refusing to flash.
  exit /b 1
)
echo Image: %MERGED_ENV%
if not exist "%PYTHON%" (
  echo %PYTHON% not found. Create the venv first, see the README.
  exit /b 1
)
rem Refuse to flash a stale image: merged.bin must be at least as new as firmware.bin
powershell -NoProfile -Command "if ((Get-Item 'merged.bin').LastWriteTime -lt (Get-Item '.pio\build\%MERGED_ENV%\firmware.bin').LastWriteTime) { exit 1 }"
if errorlevel 1 (
  echo merged.bin is OLDER than firmware.bin, or firmware.bin is missing. Run merge.cmd first.
  exit /b 1
)
echo Flashing the ESP32-S3 on %PORT%. This overwrites everything on the chip, INCLUDING saved WiFi and settings.
echo To keep them, use: deploy.cmd --keep-wifi
echo The USB-C plug orientation must select the S3 (see README). Close any serial monitor first.
"%PYTHON%" -m esptool --chip esp32s3 --port %PORT% write-flash 0x0 merged.bin
if errorlevel 1 (
  echo.
  echo FLASH FAILED. Flip the USB-C plug 180 degrees and run again.
  echo Still failing: hold BOOT, power the board on again, release, run again.
  exit /b 1
)
echo.
echo DONE. Power-cycle the board.
exit /b 0

:app_only
echo === DEPLOY: app only, keeping WiFi and settings ===
set "APP_ENV=%ENV_ARG%"
if "%APP_ENV%"=="" set "APP_ENV=%KNOB_ENV%"
rem Same guard as the full image: only knob envs, here taken from the build directory being flashed.
set "ENV_OK=0"
if /i "%APP_ENV%"=="%KNOB_ENV%" set "ENV_OK=1"
if /i "%APP_ENV%"=="%KNOB_BRINGUP_ENV%" set "ENV_OK=1"
if "%ENV_OK%"=="0" (
  echo %APP_ENV% is not a knob env. Refusing to flash. Allowed: %KNOB_ENV% or %KNOB_BRINGUP_ENV%.
  exit /b 1
)
set "APP_BIN=.pio\build\%APP_ENV%\firmware.bin"
rem build.cmd deletes the old firmware.bin before building, so a failed build leaves nothing here.
if not exist "%APP_BIN%" (
  echo No %APP_BIN%. Run build.cmd %APP_ENV% first and check it ends in BUILD OK.
  exit /b 1
)
if not exist "%PYTHON%" (
  echo %PYTHON% not found. Create the venv first, see the README.
  exit /b 1
)
echo Image: %APP_ENV%
echo Flashing ONLY the app image at %APP_OFFSET% on %PORT%. Saved WiFi and settings are kept.
echo This needs the board to already carry a full image of this layout: do a full deploy first, and again after any browser OTA update.
echo The USB-C plug orientation must select the S3 (see README). Close any serial monitor first.
"%PYTHON%" -m esptool --chip esp32s3 --port %PORT% write-flash %APP_OFFSET% "%APP_BIN%"
if errorlevel 1 (
  echo.
  echo FLASH FAILED. Flip the USB-C plug 180 degrees and run again.
  echo Still failing: hold BOOT, power the board on again, release, run again.
  exit /b 1
)
echo.
echo DONE. Power-cycle the board.
exit /b 0
