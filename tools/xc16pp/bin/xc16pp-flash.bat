@echo off
rem xc16pp-flash.bat - program the device via MPLAB IPE command line, Windows.
rem platform.txt calls: xc16pp-flash.bat <MCU> <TP> "<hex>" <run|hold>
rem   <TP>       = IPE tool code (PKOB4 / PK4 / PK5 ...), passed to ipecmd -TP
rem   <run|hold> = run -> -OL release from reset (device runs after upload);
rem                hold -> leave halted in reset (for debugging). Default: run.
rem POSITIONAL args (no "name=value"): cmd splits batch args on '=', so -mcu=...
rem would break. No Arduino bootloader; uses the real ipecmd.exe launcher.
setlocal EnableExtensions
set "MCU=%~1"
set "TP=%~2"
set "HEX=%~3"
set "RUN=%~4"
if "%MCU%"=="" echo ERROR: device required>&2& exit /b 1
if "%TP%"=="" set "TP=PKOB4"

set "RUNFLAG=-OL"
if /i "%RUN%"=="hold" set "RUNFLAG="

set "IPECMD="
for /f "delims=" %%P in ('dir /b /ad /o-n "C:\Program Files\Microchip\MPLABX\*" 2^>nul') do if not defined IPECMD if exist "C:\Program Files\Microchip\MPLABX\%%P\mplab_platform\mplab_ipe\ipecmd.exe" set "IPECMD=C:\Program Files\Microchip\MPLABX\%%P\mplab_platform\mplab_ipe\ipecmd.exe"
if not defined IPECMD echo ERROR: MPLAB X ipecmd.exe not found (install MPLAB X IDE).>&2& exit /b 2

rem ipecmd.exe resolves ipecmdboost.jar / scripts / its JRE relative to CWD, so
rem run it from its own dir (pushd) but invoke by FULL path (this cmd context does
rem not search CWD for executables). -M = program; -OL = release from reset (run).
for %%I in ("%IPECMD%") do set "IPEDIR=%%~dpI"
pushd "%IPEDIR%"
"%IPECMD%" -P%MCU% -TP%TP% -F"%HEX%" -M %RUNFLAG%
set "RC=%ERRORLEVEL%"
popd
exit /b %RC%
