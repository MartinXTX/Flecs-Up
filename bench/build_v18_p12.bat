@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Build Tier-v18 P1-2 (trivial-cache op_ctx skip) lib
if exist flecs_v18_p12.obj del flecs_v18_p12.obj
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v18_p12.c /Fo:flecs_v18_p12.obj
if errorlevel 1 exit /b 1
if exist release\v18_p12_flecs_patched.lib del release\v18_p12_flecs_patched.lib
lib /OUT:release\v18_p12_flecs_patched.lib flecs_v18_p12.obj
if errorlevel 1 exit /b 1

REM Build bench_flecs EXE with v18_p12
if exist release\v18_p12_bench_flecs.exe del release\v18_p12_bench_flecs.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include bench_flecs.c /Fe:release\v18_p12_bench_flecs.exe flecs_v18_p12.obj
if errorlevel 1 exit /b 1

echo === Tier-v18 P1-2 trivial_ctx lib + EXE built ===
dir release\v18_p12_flecs_patched.lib release\v18_p12_bench_flecs.exe