@echo off
rem ===========================================================================
rem dspic-erase.bat - Full chip erase of a dsPIC via MPLAB IPE. No IDE needed.
rem   This is the standalone "Erase Device" tool (the Arduino IDE can't rename
rem   its "Burn Bootloader" menu, so use this for a clearly-labelled erase).
rem
rem Usage:
rem   dspic-erase.bat                  (defaults: 33CK256MP508, PKOB4)
rem   dspic-erase.bat 33CK256MP508 PKOB4
rem   dspic-erase.bat 33AK128MC106 PK4
rem ===========================================================================
setlocal EnableExtensions
set "MCU=%~1"
set "TP=%~2"
if "%MCU%"=="" set "MCU=33CK256MP508"
if "%TP%"==""  set "TP=PKOB4"

set "IPECMD="
for /f "delims=" %%P in ('dir /b /ad /o-n "C:\Program Files\Microchip\MPLABX\*" 2^>nul') do if not defined IPECMD if exist "C:\Program Files\Microchip\MPLABX\%%P\mplab_platform\mplab_ipe\ipecmd.exe" set "IPECMD=C:\Program Files\Microchip\MPLABX\%%P\mplab_platform\mplab_ipe\ipecmd.exe"
if not defined IPECMD (echo ERROR: MPLAB X ipecmd.exe not found ^(install MPLAB X IDE^).& pause & exit /b 2)
for %%I in ("%IPECMD%") do set "IPEDIR=%%~dpI"

echo Erasing %MCU% via %TP% ...
pushd "%IPEDIR%"
"%IPECMD%" -P%MCU% -TP%TP% -E -OL
set "RC=%ERRORLEVEL%"
popd
if "%RC%"=="0" (echo  ^>^> Erase OK) else (echo  ^>^> Erase FAILED rc=%RC%)
pause
exit /b %RC%
