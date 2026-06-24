@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

if not exist flecs_patched_v14_1.c copy /Y flecs_patched.c.bak flecs_patched_v14_1.c >nul

if exist flecs_v14_1.obj del flecs_v14_1.obj
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v14_1.c /Fo:flecs_v14_1.obj
if errorlevel 1 exit /b 1

if exist release\bench_flecs_v14_1.exe del release\bench_flecs_v14_1.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include bench_flecs.c /Fe:release\bench_flecs_v14_1.exe flecs_v14_1.obj
if errorlevel 1 exit /b 1

echo === Tier-v14.1 reference EXE built ===
dir release\bench_flecs_v14_1.exe