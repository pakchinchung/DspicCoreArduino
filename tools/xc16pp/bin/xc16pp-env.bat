@echo off
rem ===========================================================================
rem xc16pp-env.bat — shared discovery for the Windows xc16pp-*.bat wrappers.
rem CALLed (not run standalone). Argument %1 = target device (e.g. 33CK256MP508).
rem Sets, in the CALLER's environment (no setlocal here so plain `set` persists):
rem   XCDSC, XCDSC_BIN, XC, DFP_ROOT, GLD, OUR_TC, OUR_GPP, OUR_LIBEXEC
rem Honors XCDSC_PATH and DFP_PATH overrides. Exits /b 2/3 on failure.
rem ===========================================================================
set "MCU=%~1"

rem ---- our bundled GPL C++ compiler (sits next to bin\ in the tool package) --
for %%I in ("%~dp0..\dspic-cpp-toolchain-win") do set "OUR_TC=%%~fI"
set "OUR_GPP=%OUR_TC%\bin\elf-g++.exe"
set "OUR_LIBEXEC=%OUR_TC%\libexec\gcc\pic30-elf\8.3.1"

rem ---- locate the XC-DSC / XC16 install --------------------------------------
set "XCDSC="
if defined XCDSC_PATH if exist "%XCDSC_PATH%\bin" set "XCDSC=%XCDSC_PATH%"
if not defined XCDSC for /f "delims=" %%D in ('dir /b /ad /o-n "C:\Program Files\Microchip\xc-dsc\v*" 2^>nul') do call :setifempty XCDSC "C:\Program Files\Microchip\xc-dsc\%%D"
if not defined XCDSC for /f "delims=" %%D in ('dir /b /ad /o-n "C:\Program Files\Microchip\xc16\v*" 2^>nul') do call :setifempty XCDSC "C:\Program Files\Microchip\xc16\%%D"
if not defined XCDSC echo ERROR: XC-DSC/XC16 compiler not found. Install it or set XCDSC_PATH.>&2& exit /b 2
set "XCDSC_BIN=%XCDSC%\bin"
set "XC=xc-dsc"
if exist "%XCDSC_BIN%\xc16-gcc.exe" set "XC=xc16"

rem ---- map device -> DFP family + support subdir -----------------------------
set "FAM="
set "SUPSUB="
echo %MCU%| findstr /i "CK" >nul && set "FAM=CK" && set "SUPSUB=dsPIC33C"
echo %MCU%| findstr /i "AK" >nul && set "FAM=AK" && set "SUPSUB=dsPIC33A"
if not defined FAM echo ERROR: unsupported device '%MCU%' (dsPIC33CK / dsPIC33AK only).>&2& exit /b 3
set "VAR=MP"
echo %MCU%| findstr /i "MC" >nul && set "VAR=MC"
set "DFPNAME=dsPIC33%FAM%-%VAR%_DFP"

rem ---- find the DFP under any MPLAB X install (or honor DFP_PATH) -------------
set "DFP_ROOT="
if defined DFP_PATH if exist "%DFP_PATH%" set "DFP_ROOT=%DFP_PATH%"
set "DFPBASE="
rem 1) MPLAB X packs ; 2) the standalone pack store (~/.mchp_packs) for users who
rem    downloaded the DFP without MPLAB X.
if not defined DFP_ROOT for /f "delims=" %%P in ('dir /b /ad /o-n "C:\Program Files\Microchip\MPLABX\*" 2^>nul') do call :setdfpbase "C:\Program Files\Microchip\MPLABX\%%P\packs\Microchip\%DFPNAME%"
if not defined DFPBASE call :setdfpbase "%USERPROFILE%\.mchp_packs\Microchip\%DFPNAME%"
if not defined DFP_ROOT if defined DFPBASE for /f "delims=" %%V in ('dir /b /ad /o-n "%DFPBASE%\*" 2^>nul') do call :setifempty DFP_ROOT "%DFPBASE%\%%V\xc16"
if not defined DFP_ROOT echo ERROR: DFP %DFPNAME% for '%MCU%' not found. Install via MPLAB X (Tools^>Packs) or set DFP_PATH.>&2& exit /b 3
set "GLD=%DFP_ROOT%\support\%SUPSUB%\gld\p%MCU%.gld"
exit /b 0

:setifempty
rem %1 = var name, %2 = value (quoted). Set only if the var is empty.
if not defined %~1 set "%~1=%~2"
goto :eof

:setdfpbase
rem %1 = candidate "...\Microchip\<DFPNAME>". Set DFPBASE if it exists and unset.
if not defined DFPBASE if exist "%~1" set "DFPBASE=%~1"
goto :eof
