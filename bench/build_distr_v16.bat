@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

if not exist flecs_patched_v16_distr.c (
  copy /Y ..\distr\flecs.c.tier_v16 flecs_patched_v16_distr.c >nul
)

if exist flecs_v16_distr.obj del flecs_v16_distr.obj
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I ..\distr flecs_patched_v16_distr.c /Fo:flecs_v16_distr.obj
if errorlevel 1 exit /b 1
lib /OUT:flecs_v16_distr.lib flecs_v16_distr.obj
if errorlevel 1 exit /b 1

echo === Tier-v16 distr built ===
dir flecs_v16_distr.lib