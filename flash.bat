@echo off
setlocal

:: ── config ──────────────────────────────────────────────
set "ENV=puzzles"
set "MONITOR_SPEED=115200"
:: ─────────────────────────────────────────────────────────

if "%1"=="" goto :flash
if /i "%1"=="build"  goto :build
if /i "%1"=="flash"   goto :flash
if /i "%1"=="monitor" goto :monitor
if /i "%1"=="clean"   goto :clean
goto :usage

:build
echo === Building %ENV% ===
pio run -e %ENV%
if errorlevel 1 ( echo BUILD FAILED & exit /b 1 )
echo === Build OK ===
exit /b 0

:flash
echo === Building + flashing %ENV% ===
pio run -e %ENV% -t upload
if errorlevel 1 ( echo FLASH FAILED & exit /b 1 )
echo === Flash OK ===
if "%2"=="--monitor" goto :monitor
exit /b 0

:monitor
echo === Serial monitor at %MONITOR_SPEED% baud ===
pio device monitor -b %MONITOR_SPEED%
exit /b 0

:clean
echo === Cleaning %ENV% ===
pio run -e %ENV% -t clean
exit /b 0

:usage
echo Usage: %~nx0 [build^|flash^|monitor^|clean]
echo.
echo   (default)  build + flash
echo   build      compile only
echo   flash      build + upload to device
echo   flash --monitor  flash then open serial monitor
echo   monitor    open serial monitor
echo   clean      remove build artifacts
exit /b 1
