@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
  echo VCVARS FAILED
  exit /b 1
)
echo === Building test_stage_v2 ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c test_stage.c /Fo:test_stage_v2.obj
if errorlevel 1 exit /b 1
link /OUT:test_stage_v2.exe test_stage_v2.obj release\flecs_patched.lib
if errorlevel 1 exit /b 1
echo === BUILD OK ===