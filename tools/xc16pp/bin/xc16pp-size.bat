@echo off
rem xc16pp-size.bat - report section sizes, Windows.
rem platform.txt calls: xc16pp-size.bat "in.elf"
setlocal EnableExtensions
for /f "delims=" %%D in ('dir /b /ad /o-n "C:\Program Files\Microchip\xc-dsc\v*" 2^>nul') do if not defined XCDSC_BIN set "XCDSC_BIN=C:\Program Files\Microchip\xc-dsc\%%D\bin"
if not defined XCDSC_BIN echo ERROR: XC-DSC not found.>&2& exit /b 2
set "XC=xc-dsc"
if exist "%XCDSC_BIN%\xc16-objdump.exe" set "XC=xc16"
"%XCDSC_BIN%\%XC%-objdump.exe" --section-headers %*
exit /b %ERRORLEVEL%
