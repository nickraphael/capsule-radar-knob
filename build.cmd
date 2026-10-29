@echo off
setlocal
cd /d "%~dp0"
call settings.cmd
set "ENV_NAME=%KNOB_ENV%"
if not "%~1"=="" set "ENV_NAME=%~1"
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
docker run --rm -v "%cd%:/project" -v %VOL_CACHE%:/root/.platformio -v %VOL_WORK%:/pio_work -e PLATFORMIO_WORKSPACE_DIR=/pio_work -w /project %IMAGE% bash scripts/docker_build.sh %ENV_NAME%
if errorlevel 1 (
  echo.
  echo BUILD FAILED. Do not merge or flash.
  exit /b 1
)
echo.
echo BUILD OK. Next: merge.cmd
