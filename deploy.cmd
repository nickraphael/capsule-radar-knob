@echo off
setlocal
cd /d "%~dp0"
call settings.cmd
echo === DEPLOY ===
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
echo Flashing the ESP32-S3 on %PORT%. This overwrites everything on the chip.
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
