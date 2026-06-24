@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Build bench_dispatch against Tier-v17
if exist bench_dispatch_v17.obj del bench_dispatch_v17.obj
if exist bench_dispatch_v17.exe del bench_dispatch_v17.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c bench_dispatch.c /Fo:bench_dispatch_v17.obj
if errorlevel 1 exit /b 1
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include bench_dispatch_v17.obj flecs_v17.obj /Fe:bench_dispatch_v17.exe
if errorlevel 1 exit /b 1

REM Build bench_dispatch against Tier-v16
if exist bench_dispatch_v16.obj del bench_dispatch_v16.obj
if exist bench_dispatch_v16.exe del bench_dispatch_v16.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c bench_dispatch.c /Fo:bench_dispatch_v16.obj
if errorlevel 1 exit /b 1
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include bench_dispatch_v16.obj flecs_v16.obj /Fe:bench_dispatch_v16.exe
if errorlevel 1 exit /b 1

echo === bench_dispatch built for v16 and v17 ===
dir bench_dispatch_v17.exe bench_dispatch_v16.exe