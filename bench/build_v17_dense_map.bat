@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Tier-v17 P0-3 dense_map build
if exist flecs_v17.obj del flecs_v17.obj
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c flecs_patched_v17.c /Fo:flecs_v17.obj
if errorlevel 1 (
  echo === v17 dense_map build FAILED ===
  exit /b 1
)
echo === v17 dense_map build OK ===