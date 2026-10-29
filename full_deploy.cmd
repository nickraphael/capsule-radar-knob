@echo off
setlocal
cd /d "%~dp0"

rem Usage: full_deploy.cmd [--keep-wifi] [env]
rem   (default)     build, merge, flash the FULL image (erases saved WiFi and settings)
rem   --keep-wifi   build, then flash ONLY the app image: no merge step, WiFi and settings survive
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

if "%KEEP_WIFI%"=="1" (
  echo ===== FULL DEPLOY, keeping WiFi: build then flash the app only =====
) else (
  echo ===== FULL DEPLOY: build, merge, flash =====
)
echo.
call build.cmd %ENV_ARG%
if errorlevel 1 goto :failed
echo.
if "%KEEP_WIFI%"=="1" goto :keep
call merge.cmd %ENV_ARG%
if errorlevel 1 goto :failed
echo.
call deploy.cmd
if errorlevel 1 goto :failed
goto :ok

:keep
call deploy.cmd --keep-wifi %ENV_ARG%
if errorlevel 1 goto :failed

:ok
echo.
echo ===== FULL DEPLOY COMPLETE =====
exit /b 0

:failed
echo.
echo ===== FULL DEPLOY STOPPED: a step failed (see above) =====
exit /b 1
