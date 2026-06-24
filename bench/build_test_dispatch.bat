@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Build Tier-v17 P2-3 dispatcher test
if exist test_dispatch_v17.obj del test_dispatch_v17.obj
if exist test_dispatch_v17.exe del test_dispatch_v17.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c test_dispatch_v17.c /Fo:test_dispatch_v17.obj
if errorlevel 1 exit /b 1
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_dispatch_v17.obj flecs_v17.obj /Fe:test_dispatch_v17.exe
if errorlevel 1 exit /b 1

echo === Tier-v17 P2-3 dispatcher test built ===
test_dispatch_v17.exe