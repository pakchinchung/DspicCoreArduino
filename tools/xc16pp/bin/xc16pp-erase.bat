@echo off
rem xc16pp-erase.bat - FULL CHIP ERASE via MPLAB IPE command line, Windows.
rem This core has no bootloader, so the Arduino IDE "Tools > Burn Bootloader"
rem action is repurposed to erase the device's flash.
rem platform.txt bootloader hook calls: xc16pp-erase.bat <MCU> <TP>
rem   <TP> = IPE tool code (PKOB4 / PK4 / PK5 ...). POSITIONAL args (cmd splits
rem          batch args on '=', so -mcu=... would break).
setlocal EnableExtensions
set "MCU=%~1"
set "TP=%~2"
if "%MCU%"=="" echo ERROR: device required>&2& exit /b 1
if "%TP%"=="" set "TP=PKOB4"

set "IPECMD="
for /f "delims=" %%P in ('dir /b /ad /o-n "C:\Program Files\Microchip\MPLABX\*" 2^>nul') do if not defined IPECMD if exist "C:\Program Files\Microchip\MPLABX\%%P\mplab_platform\mplab_ipe\ipecmd.exe" set "IPECMD=C:\Program Files\Microchip\MPLABX\%%P\mplab_platform\mplab_ipe\ipecmd.exe"
if not defined IPECMD echo ERROR: MPLAB X ipecmd.exe not found (install MPLAB X IDE).>&2& exit /b 2

rem ipecmd resolves its JRE/scripts relative to CWD, so run from its own dir
rem (pushd) but invoke by full path. -E = Erase Flash Device; -OL = release reset.
for %%I in ("%IPECMD%") do set "IPEDIR=%%~dpI"
pushd "%IPEDIR%"
"%IPECMD%" -P%MCU% -TP%TP% -E -OL
set "RC=%ERRORLEVEL%"
popd
exit /b %RC%
