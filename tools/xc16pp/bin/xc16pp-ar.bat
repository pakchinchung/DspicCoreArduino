@echo off
rem xc16pp-ar.bat - build the core static library, Windows.
rem platform.txt calls: xc16pp-ar.bat <ar flags> "archive" "object"
rem No device info needed; just the stock archiver.
setlocal EnableExtensions
call "%~dp0xc16pp-env.bat" "33CK256MP508" >nul 2>&1
if not defined XCDSC_BIN for /f "delims=" %%D in ('dir /b /ad /o-n "C:\Program Files\Microchip\xc-dsc\v*" 2^>nul') do if not defined XCDSC_BIN set "XCDSC_BIN=C:\Program Files\Microchip\xc-dsc\%%D\bin"
if not defined XC set "XC=xc-dsc"
"%XCDSC_BIN%\%XC%-ar.exe" %*
exit /b %ERRORLEVEL%
