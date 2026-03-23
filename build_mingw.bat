@echo off
setlocal

if not exist regdemo.c goto :missing
if not exist regdemo.rc goto :missing
if not exist regdemo.ico goto :missing

windres regdemo.rc -O coff -o regdemo_res.o
if errorlevel 1 goto :eof

gcc -mwindows -Os -s -DWINVER=0x0400 -D_WIN32_WINNT=0x0400 -o regdemo.exe regdemo.c regdemo_res.o -ladvapi32 -lgdi32 -luser32 -Wl,--major-subsystem-version,4,--minor-subsystem-version,0
if errorlevel 1 goto :eof

echo Built regdemo.exe
exit /b 0

:missing
echo Missing source files in current directory.
exit /b 1
