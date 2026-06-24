@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Build bench with v17 lib (control)
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include bench_trivial_ctx.c /Fe:release\bench_trivial_v17.exe release\v17_flecs_patched.lib
if errorlevel 1 exit /b 1

REM Build bench with v18_p12 lib (P1-2 patched)
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include bench_trivial_ctx.c /Fe:release\bench_trivial_v18p12.exe release\v18_p12_flecs_patched.lib
if errorlevel 1 exit /b 1

echo === bench_trivial_ctx binaries built ===
dir release\bench_trivial_v17.exe release\bench_trivial_v18p12.exe