@echo off
rem xc16pp-size.bat - report section sizes, Windows.
rem platform.txt calls: xc16pp-size.bat <mcu> "in.elf"
rem -mdfp is passed to objdump to suppress its "c30_device.info" lookup warning.
setlocal EnableExtensions
set "MCU=%~1"
call "%~dp0xc16pp-env.bat" "%MCU%" >nul 2>&1
set "REST=%*"
call set "REST=%%REST:*%MCU% =%%"
if defined DFP_ROOT (
  "%XCDSC_BIN%\%XC%-objdump.exe" -mdfp="%DFP_ROOT%" --section-headers %REST%
) else (
  for /f "delims=" %%D in ('dir /b /ad /o-n "C:\Program Files\Microchip\xc-dsc\v*" 2^>nul') do if not defined XCDSC_BIN set "XCDSC_BIN=C:\Program Files\Microchip\xc-dsc\%%D\bin"
  "%XCDSC_BIN%\xc-dsc-objdump.exe" --section-headers %REST%
)
exit /b %ERRORLEVEL%
