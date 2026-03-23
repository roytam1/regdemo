@echo off
setlocal

if not exist regdemo.c goto :missing
if not exist regdemo.rc goto :missing
if not exist regdemo.ico goto :missing

rc /r regdemo.rc
if errorlevel 1 goto :eof

cl /nologo /W4 /O1 /TC /DWINVER=0x0400 /D_WIN32_WINNT=0x0400 regdemo.c regdemo.res user32.lib gdi32.lib advapi32.lib /link /SUBSYSTEM:WINDOWS,4.0 /MACHINE:IX86 /OUT:regdemo.exe
if errorlevel 1 goto :eof

echo Built regdemo.exe
exit /b 0

:missing
echo Missing source files in current directory.
exit /b 1
