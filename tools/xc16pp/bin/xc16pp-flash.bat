@echo off
rem xc16pp-flash.bat - program the device via MPLAB IPE command line, Windows.
rem platform.txt calls: xc16pp-flash.bat -mcu=<DEV> -tp=<PKOB|PK4|PK5> -hex="..." [-v4|-v0]
rem -tp is the IPE tool code passed straight to ipecmd -TP. No bootloader.
rem Not exercised in CI - needs a connected on-board PICkit / PICkit 4 / PICkit 5.
setlocal EnableExtensions EnableDelayedExpansion
set "MCU=" & set "TP=PK4" & set "HEX=" & set "VERBOSE="
:parse
if "%~1"=="" goto find
set "A=%~1"
if /i "!A:~0,5!"=="-mcu=" set "MCU=!A:~5!"
if /i "!A:~0,4!"=="-tp=" set "TP=!A:~4!"
if /i "!A:~0,5!"=="-hex=" set "HEX=!A:~5!"
if /i "!A!"=="-v4" set "VERBOSE=-v4"
if /i "!A!"=="-v0" set "VERBOSE=-v0"
shift
goto parse

:find
set "IPECMD="
for /f "delims=" %%P in ('dir /b /ad /o-n "C:\Program Files\Microchip\MPLABX\*" 2^>nul') do if not defined IPECMD if exist "C:\Program Files\Microchip\MPLABX\%%P\mplab_platform\mplab_ipe\ipecmd.jar" set "IPECMD=C:\Program Files\Microchip\MPLABX\%%P\mplab_platform\mplab_ipe\ipecmd.jar"
if not defined IPECMD echo ERROR: MPLAB X ipecmd.jar not found.>&2& exit /b 2
java -jar "%IPECMD%" -P%MCU% -TP%TP% -F"%HEX%" -M %VERBOSE%
exit /b %ERRORLEVEL%
