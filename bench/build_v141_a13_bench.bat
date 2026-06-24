@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Same Tier-A1.3 LAZY benchmark but linked against Tier-v14.1 lib
REM (which does NOT have ecs_world_auto_dont_fragment, so the call is a no-op stub
REM  and the LAZY path falls through to archetype storage.)

if exist bench_tier_a1_3_lazy_v141.obj del bench_tier_a1_3_lazy_v141.obj
if exist bench_tier_a1_3_lazy_v141.exe del bench_tier_a1_3_lazy_v141.exe

cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c bench_tier_a1_3_lazy.c /Fo:bench_tier_a1_3_lazy_v141.obj
if errorlevel 1 exit /b 1

link /OUT:bench_tier_a1_3_lazy_v141.exe bench_tier_a1_3_lazy_v141.obj release\flecs_patched_v14_1.lib
if errorlevel 1 exit /b 1

echo === Running Tier-A1.3 LAZY benchmark against Tier-v14.1 lib ===
"C:\Project\Flecs\bench\bench_tier_a1_3_lazy_v141.exe" 100 1000
echo === Exit %errorlevel% ===
