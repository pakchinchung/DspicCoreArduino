@echo off
rem ===========================================================================
rem dspic-tool.bat - interactive dsPIC programmer (no Arduino IDE recompile).
rem   Menu options:
rem     [1] Burn LAST compiled hex   - flash the most recently built sketch
rem     [2] Mass-program last hex    - loop, flashing board after board
rem     [3] Erase device             - full chip erase
rem     [4] Burn a chosen .hex file
rem     [5] Change device / tool code
rem   Uses MPLAB IPE (ipecmd). "Last hex" is found in the Arduino build cache,
rem   so just Verify/Compile the sketch once in the IDE, then come here.
rem ===========================================================================
setlocal EnableExtensions EnableDelayedExpansion
title dsPIC Programmer Tool

set "MCU=33CK256MP508"
set "TP=PKOB4"
set "CACHE=%LOCALAPPDATA%\arduino\sketches"

rem --- locate MPLAB IPE ipecmd.exe ---
set "IPECMD="
for /f "delims=" %%P in ('dir /b /ad /o-n "C:\Program Files\Microchip\MPLABX\*" 2^>nul') do if not defined IPECMD if exist "C:\Program Files\Microchip\MPLABX\%%P\mplab_platform\mplab_ipe\ipecmd.exe" set "IPECMD=C:\Program Files\Microchip\MPLABX\%%P\mplab_platform\mplab_ipe\ipecmd.exe"
if not defined IPECMD (echo ERROR: MPLAB X ipecmd.exe not found ^(install MPLAB X IDE^).& pause & goto :eof)
for %%I in ("%IPECMD%") do set "IPEDIR=%%~dpI"

:menu
cls
echo ==================================================
echo   dsPIC Programmer    device=%MCU%  tool=%TP%
echo ==================================================
echo    [1] Burn LAST compiled hex (no recompile)
echo    [2] Mass-program last hex (loop, many boards)
echo    [3] Erase device
echo    [4] Burn a chosen .hex file
echo    [5] Change device / tool code
echo    [Q] Quit
echo.
set "C="
set /p "C=Choose: "
if /i "%C%"=="1" ( call :findlast && call :program "!LASTHEX!" & pause & goto menu )
if /i "%C%"=="2" ( call :findlast && call :massloop "!LASTHEX!" & goto menu )
if /i "%C%"=="3" ( call :erase & pause & goto menu )
if /i "%C%"=="4" ( call :pickhex & goto menu )
if /i "%C%"=="5" ( call :changecfg & goto menu )
if /i "%C%"=="Q" goto :eof
goto menu

rem -------------------------------------------------------------- subroutines
:findlast
rem Newest *.ino.hex anywhere under the Arduino build cache -> LASTHEX.
set "LASTHEX="
for /f "delims=" %%F in ('powershell -NoProfile -Command "Get-ChildItem -Path '%CACHE%' -Recurse -Filter *.ino.hex -ErrorAction SilentlyContinue ^| Sort-Object LastWriteTime -Descending ^| Select-Object -First 1 -ExpandProperty FullName" 2^>nul') do set "LASTHEX=%%F"
if not defined LASTHEX (
    echo No compiled .hex found in the Arduino build cache.
    echo Compile a sketch once in the IDE ^(Verify^), then try again.
    pause
    exit /b 1
)
echo Last compiled hex:
echo   !LASTHEX!
exit /b 0

:program
rem %1 = hex path. Programs once. -M auto-erases the program region; -OL runs it.
set "H=%~1"
if not exist "%H%" (echo ERROR: hex not found: "%H%"& exit /b 1)
echo Programming %MCU% via %TP% ...
pushd "%IPEDIR%"
"%IPECMD%" -P%MCU% -TP%TP% -F"%H%" -M -OL
set "RC=!ERRORLEVEL!"
popd
if "!RC!"=="0" (echo  ^>^> OK) else (echo  ^>^> FAILED rc=!RC!)
exit /b !RC!

:massloop
set "H=%~1"
set /a OK=0, FAIL=0, N=0
:massnext
set /a N+=1
echo.
echo [#!N!] Programming...
pushd "%IPEDIR%"
"%IPECMD%" -P%MCU% -TP%TP% -F"%H%" -M -OL
set "RC=!ERRORLEVEL!"
popd
if "!RC!"=="0" (set /a OK+=1 & echo    ^>^> OK) else (set /a FAIL+=1 & echo    ^>^> FAILED rc=!RC!)
echo    Totals: !OK! ok / !FAIL! failed
set "A="
set /p "A=Swap board + ENTER to program next (Q + ENTER to stop): "
if /i not "!A!"=="Q" goto massnext
exit /b 0

:erase
echo Erasing %MCU% via %TP% ...
pushd "%IPEDIR%"
"%IPECMD%" -P%MCU% -TP%TP% -E -OL
set "RC=!ERRORLEVEL!"
popd
if "!RC!"=="0" (echo  ^>^> Erase OK) else (echo  ^>^> Erase FAILED rc=!RC!)
exit /b !RC!

:pickhex
set "H="
set /p "H=Path to .hex (drag it here): "
set "H=!H:"=!"
if not exist "!H!" (echo ERROR: file not found.& pause & exit /b 1)
call :program "!H!"
pause
exit /b 0

:changecfg
set /p "MCU=Device [%MCU%]: " || rem keep
set /p "TP=Tool code (PKOB4/PK4/PK5/ICD4/SNAP) [%TP%]: " || rem keep
exit /b 0
