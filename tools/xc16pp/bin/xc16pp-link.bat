@echo off
rem xc16pp-link.bat - link objects + core archive into an ELF, Windows.
rem platform.txt calls: xc16pp-link.bat MCU <ld flags> -o out.elf objs... -lm
rem Driven through stock xc-dsc-gcc; the device .gld MUST be passed with -T or the
rem linker defaults to the wrong 30Fxxxx arch.
setlocal EnableExtensions
set "MCU=%~1"
call "%~dp0xc16pp-env.bat" "%MCU%"
if errorlevel 1 exit /b 1
set "REST=%*"
call set "REST=%%REST:*%MCU% =%%"
"%XCDSC_BIN%\%XC%-gcc.exe" -mcpu=%MCU% -mdfp="%DFP_ROOT%" -T"%GLD%" %REST%
exit /b %ERRORLEVEL%
