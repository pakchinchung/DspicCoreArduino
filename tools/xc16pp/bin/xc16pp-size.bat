@echo off
rem xc16pp-size.bat - report program/data usage in DECIMAL, Windows.
rem platform.txt calls: xc16pp-size.bat <mcu> "in.elf"
rem
rem objdump prints section sizes in HEX; arduino-cli's recipe.size.regex sums in
rem DECIMAL, so we sum the hex sizes here (set /a 0x..) by category and print
rem decimals:  "Program bytes: N" / "Data bytes: N".
rem
rem IMPORTANT: objdump MUST be given -mdfp=<device pack> so it reports sizes in
rem the device's program-memory units. Without it, program sections are reported
rem in raw object bytes (~2x too large) and the figure won't match MPLAB X.
setlocal EnableExtensions EnableDelayedExpansion
set "MCU=%~1"
set "ELF=%~2"

rem ---- discovery (XCDSC_BIN, XC, DFP_ROOT) via the shared env helper ----
call "%~dp0xc16pp-env.bat" "%MCU%" >nul 2>&1

rem Fallbacks so the size step degrades gracefully instead of erroring.
if not defined XCDSC_BIN for /f "delims=" %%D in ('dir /b /ad /o-n "C:\Program Files\Microchip\xc-dsc\v*" 2^>nul') do if not defined XCDSC_BIN set "XCDSC_BIN=C:\Program Files\Microchip\xc-dsc\%%D\bin"
if not defined XC set "XC=xc-dsc"
rem DFP_ROOT contains spaces ("C:\Program Files\..."), so embed quotes around the
rem path; %MDFP% is expanded UNQUOTED below and cmd strips these inner quotes,
rem delivering -mdfp=<path with spaces> to objdump as a single argument.
set "MDFP="
if defined DFP_ROOT set MDFP=-mdfp="%DFP_ROOT%"

if not exist "%XCDSC_BIN%\%XC%-objdump.exe" (
    echo Program bytes: 0
    echo Data bytes: 0
    endlocal & exit /b 0
)

rem ---- dump section headers to a temp file, then sum hex sizes by category ----
set "SECTMP=%TEMP%\dspic_size_%RANDOM%.txt"
"%XCDSC_BIN%\%XC%-objdump.exe" %MDFP% --section-headers "%ELF%" > "%SECTMP%" 2>nul

set /a PROG=0, DATA=0
for /f "usebackq tokens=2,3" %%a in ("%SECTMP%") do (
    set "nm=%%a"
    set "sz=%%b"
    if "!nm:~0,5!"==".text"  set /a PROG+=0x!sz!
    if "!nm:~0,6!"==".const" set /a PROG+=0x!sz!
    if "!nm:~0,6!"==".dinit" set /a PROG+=0x!sz!
    if "!nm:~0,6!"==".reset" set /a PROG+=0x!sz!
    if "!nm:~0,5!"==".aivt"  set /a PROG+=0x!sz!
    if "!nm:~0,4!"==".ivt"   set /a PROG+=0x!sz!
    if "!nm:~0,4!"==".isr"   set /a PROG+=0x!sz!
    if "!nm:~0,6!"==".ndata" set /a DATA+=0x!sz!
    if "!nm:~0,5!"==".nbss"  set /a DATA+=0x!sz!
    if "!nm:~0,5!"==".data"  set /a DATA+=0x!sz!
    if "!nm:~0,4!"==".bss"   set /a DATA+=0x!sz!
)
del "%SECTMP%" 2>nul
echo Program bytes: !PROG!
echo Data bytes: !DATA!
endlocal
