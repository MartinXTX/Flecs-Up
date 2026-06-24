@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Tier-A1.3 LAZY benchmark build
REM Uses release/flecs_patched.lib (Tier-v16 + Tier-A1.3 fix) as static lib

if exist bench_tier_a1_3_lazy.obj del bench_tier_a1_3_lazy.obj
if exist bench_tier_a1_3_lazy.exe del bench_tier_a1_3_lazy.exe

cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c bench_tier_a1_3_lazy.c /Fo:bench_tier_a1_3_lazy.obj
if errorlevel 1 exit /b 1

link /OUT:bench_tier_a1_3_lazy.exe bench_tier_a1_3_lazy.obj release\flecs_patched.lib
if errorlevel 1 exit /b 1

echo === Running Tier-A1.3 LAZY benchmark ===
"C:\Project\Flecs\bench\bench_tier_a1_3_lazy.exe" 100 1000
echo === Exit %errorlevel% ===
