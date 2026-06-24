@echo off
rem ===========================================================================
rem dspic-prog.bat - Mass-program a PREBUILT .hex onto dsPIC via MPLAB IPE.
rem   No Arduino IDE, no recompile. Compile your sketch ONCE (IDE: Sketch >
rem   "Export Compiled Binary", or `arduino-cli compile --export-binaries`),
rem   then use this to flash chip after chip from the same .hex.
rem
rem Usage:
rem   dspic-prog.bat                         (prompts for the .hex, then loops)
rem   dspic-prog.bat "path\to\sketch.ino.hex"
rem   dspic-prog.bat "path\to\sketch.ino.hex" 33CK256MP508 PKOB4
rem   - or just DRAG a .hex file onto this .bat in Explorer.
rem
rem Defaults: device 33CK256MP508, tool PKOB4 (on-board PICkit). Override with
rem the 2nd/3rd args (e.g. PK4 / PK5 for an external programmer).
rem ===========================================================================
setlocal EnableExtensions EnableDelayedExpansion
set "HEX=%~1"
set "MCU=%~2"
set "TP=%~3"
if "%MCU%"=="" set "MCU=33CK256MP508"
if "%TP%"==""  set "TP=PKOB4"

if "%HEX%"=="" set /p "HEX=Path to .hex file (drag it here): "
set "HEX=%HEX:"=%"
if not exist "%HEX%" (echo ERROR: file not found: "%HEX%"& pause & exit /b 1)

rem --- locate MPLAB IPE ipecmd.exe ---
set "IPECMD="
for /f "delims=" %%P in ('dir /b /ad /o-n "C:\Program Files\Microchip\MPLABX\*" 2^>nul') do if not defined IPECMD if exist "C:\Program Files\Microchip\MPLABX\%%P\mplab_platform\mplab_ipe\ipecmd.exe" set "IPECMD=C:\Program Files\Microchip\MPLABX\%%P\mplab_platform\mplab_ipe\ipecmd.exe"
if not defined IPECMD (echo ERROR: MPLAB X ipecmd.exe not found ^(install MPLAB X IDE^).& pause & exit /b 2)
for %%I in ("%IPECMD%") do set "IPEDIR=%%~dpI"

echo ============================================================
echo  Device : %MCU%
echo  Tool   : %TP%
echo  Hex    : %HEX%
echo ============================================================
set /a COUNT=0, OK=0, FAIL=0

:loop
set /a COUNT+=1
echo.
echo [#!COUNT!] Programming...
pushd "%IPEDIR%"
rem -M programs (auto-erases the program region first); -OL releases from reset.
"%IPECMD%" -P%MCU% -TP%TP% -F"%HEX%" -M -OL
set "RC=!ERRORLEVEL!"
popd
if "!RC!"=="0" (set /a OK+=1 & echo    ^>^> OK) else (set /a FAIL+=1 & echo    ^>^> FAILED ^(rc=!RC!^))
echo    Totals: !OK! ok / !FAIL! failed of !COUNT!
echo.
set "ANS="
set /p "ANS=Swap to the next board, press ENTER to program (or type Q + ENTER to quit): "
if /i not "!ANS!"=="Q" goto loop

echo.
echo Done. Programmed !OK! board(s) ok, !FAIL! failed.
endlocal
