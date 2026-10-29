@echo off
setlocal
cd /d "%~dp0"
call settings.cmd
set "ENV_NAME=%KNOB_ENV%"
if not "%~1"=="" set "ENV_NAME=%~1"
echo === MERGE: %ENV_NAME% ===
if not exist ".pio\build\%ENV_NAME%\firmware.bin" (
  echo No firmware.bin for %ENV_NAME%. Run build.cmd first.
  exit /b 1
)
if exist merged.bin del merged.bin
if exist ".pio\merged_env.txt" del ".pio\merged_env.txt"
docker run --rm -v "%cd%:/project" -v %VOL_CACHE%:/root/.platformio -w /project %IMAGE% bash scripts/docker_merge.sh %ENV_NAME%
if errorlevel 1 (
  echo.
  echo MERGE FAILED. Do not flash.
  exit /b 1
)
if not exist merged.bin (
  echo.
  echo MERGE FAILED: merged.bin was not created.
  exit /b 1
)
rem Record which env this image was built for. deploy.cmd refuses anything not built for KNOB_ENV.
(echo %ENV_NAME%)>".pio\merged_env.txt"
echo.
echo MERGE OK. Next: deploy.cmd
