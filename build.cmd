@echo off
setlocal
cd /d "%~dp0"
call settings.cmd
set "ENV_NAME=%KNOB_ENV%"

rem Arguments: an optional env name; --keep-wifi is accepted but only matters when flashing.
:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="--keep-wifi" (
  echo Note: --keep-wifi only affects flashing. Use it with deploy.cmd or full_deploy.cmd. Ignored here.
) else (
  set "ENV_NAME=%~1"
)
shift
goto :parse_args
:args_done

echo === BUILD: %ENV_NAME% ===
docker info >nul 2>&1
if errorlevel 1 (
  echo Docker is not running. Start Docker Desktop, wait for the engine, then retry.
  exit /b 1
)
docker image inspect %IMAGE% >nul 2>&1
if errorlevel 1 (
  echo Building the %IMAGE% image. One-off, needs internet.
  docker build -t %IMAGE% - < Dockerfile
  if errorlevel 1 (
    echo.
    echo IMAGE BUILD FAILED.
    exit /b 1
  )
)
rem Remove the previous copy first: if this build fails there is then NO firmware.bin to flash by mistake.
if exist ".pio\build\%ENV_NAME%\firmware.bin" del ".pio\build\%ENV_NAME%\firmware.bin"
docker run --rm -v "%cd%:/project" -v %VOL_CACHE%:/root/.platformio -v %VOL_WORK%:/pio_work -e PLATFORMIO_WORKSPACE_DIR=/pio_work -w /project %IMAGE% bash scripts/docker_build.sh %ENV_NAME%
if errorlevel 1 (
  echo.
  echo BUILD FAILED. Do not merge or flash.
  exit /b 1
)
echo.
echo BUILD OK. Next: merge.cmd then deploy.cmd, or deploy.cmd --keep-wifi to flash only the app.
