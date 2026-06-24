@echo off
cd /d "C:\Project\Flecs\distr"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Restore distr/flecs.c from Tier-v16 source with flecs.h include
if exist ..\bench\flecs_patched_v16.c (
  copy /Y ..\bench\flecs_patched_v16.c flecs.c >nul
  powershell -Command "(Get-Content flecs.c) -replace '#include \"flecs_patched.h\"', '#include \"flecs.h\"' | Set-Content flecs.c"
)

if exist flecs_v16_distr.obj del flecs_v16_distr.obj
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . flecs.c /Fo:flecs_v16_distr.obj
if errorlevel 1 exit /b 1
lib /OUT:flecs_v16_distr.lib flecs_v16_distr.obj
if errorlevel 1 exit /b 1

echo === Tier-v16 distr built ===
dir flecs_v16_distr.lib