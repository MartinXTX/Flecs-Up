@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >/dev/null 2>&1

REM Build Tier-v17 (P2-2 arena consolidation) lib + bench
if exist flecs_v17.obj del flecs_v17.obj
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v17.c /Fo:flecs_v17.obj
if errorlevel 1 exit /b 1
lib /OUT:release\v17_flecs_patched.lib flecs_v17.obj
if errorlevel 1 exit /b 1

REM Build benchmark EXE
if exist release\v17_bench_flecs.exe del release\v17_bench_flecs.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include bench_flecs.c /Fe:release\v17_bench_flecs.exe flecs_v17.obj
if errorlevel 1 exit /b 1

echo === Tier-v17 arena lib + EXE built ===
dir release\v17_flecs_patched.lib release\v17_bench_flecs.exe
