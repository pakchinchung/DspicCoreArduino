@echo off
rem xc16pp-hex.bat — ELF -> Intel HEX (Windows).
rem Called by platform.txt: xc16pp-hex.bat <mcu> "in.elf" "out.hex"
rem bin2hex needs -mdfp to read the device's memory layout (c30_device.info).
setlocal EnableExtensions
set "MCU=%~1"
set "INELF=%~2"
set "OUTHEX=%~3"
call "%~dp0xc16pp-env.bat" "%MCU%" || exit /b 1
"%XCDSC_BIN%\%XC%-bin2hex.exe" -mdfp="%DFP_ROOT%" "%INELF%" -omf=elf
exit /b %ERRORLEVEL%
