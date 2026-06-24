@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Rebuild test_observer against v17 lib
if exist test_observer_v17.exe del test_observer_v17.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_observer.c flecs_v17.obj /Fe:test_observer_v17.exe
if errorlevel 1 exit /b 1
test_observer_v17.exe