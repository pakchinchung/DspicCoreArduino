@echo off
rem xc16pp-compile.bat - Windows C/C++/asm front-end for dsPIC, no shell needed.
rem platform.txt calls: xc16pp-compile.bat LANG MCU <flags> "src" -o "obj"
rem   c   -^> stock xc-dsc-gcc
rem   cpp -^> our elf-g++, which drives the stock xc-dsc-as found via -B
rem   asm -^> stock xc-dsc-as
setlocal EnableExtensions
set "LANG=%~1"
set "MCU=%~2"
call "%~dp0xc16pp-env.bat" "%MCU%"
if errorlevel 1 exit /b 1

rem Strip the leading "LANG MCU " from the full arg string, keeping the rest
rem verbatim with its original quoting (no per-token re-quoting needed).
set "REST=%*"
call set "REST=%%REST:*%MCU% =%%"

if /i "%LANG%"=="c"   goto do_c
if /i "%LANG%"=="cpp" goto do_cpp
if /i "%LANG%"=="asm" goto do_asm
echo ERROR: -lang must be c, cpp or asm [got '%LANG%']>&2
exit /b 1

:do_c
"%XCDSC_BIN%\%XC%-gcc.exe" -mcpu=%MCU% -mdfp="%DFP_ROOT%" %REST%
exit /b %ERRORLEVEL%

:do_cpp
rem -B our libexec finds cc1plus; -B XC-DSC bin finds xc-dsc-as so one -c makes
rem the .o. -I XC-DSC\include supplies target stdint.h etc. The C++ compat shim
rem is added by platform.txt via -include.
"%OUR_GPP%" -mcpu=%MCU% -mdfp="%DFP_ROOT%" -B"%OUR_LIBEXEC%" -B"%XCDSC_BIN%" -I"%XCDSC%\include" %REST%
exit /b %ERRORLEVEL%

:do_asm
"%XCDSC_BIN%\%XC%-as.exe" -mcpu=%MCU% -mdfp="%DFP_ROOT%" %REST%
exit /b %ERRORLEVEL%
