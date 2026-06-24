@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Build v16 lib first
if not exist release\v16_flecs_patched.lib (
  if not exist flecs_v16.obj cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v16.c /Fo:flecs_v16.obj
  lib /OUT:release\v16_flecs_patched.lib flecs_v16.obj
)

REM Build MT test with v16 lib
if exist test_v14_1_deep_v14_mt_v16.exe del test_v14_1_deep_v14_mt_v16.exe
cl /O2 /EHsc /std:c++20 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_v14_1_deep_v14_mt.cpp /Fe:test_v14_1_deep_v14_mt_v16.exe release\v16_flecs_patched.lib
if errorlevel 1 exit /b 1

echo === MT test build OK ===
echo Running 5x stability against v16 lib...
for /L %%i in (1,1,5) do (
  echo === Run %%i ===
  "C:\Project\Flecs\bench\test_v14_1_deep_v14_mt_v16.exe" > mt_run_%%i_out.txt 2>&1
  findstr /B /C:"PASS" /C:"FAIL" mt_run_%%i_out.txt | findstr /V "MSVC"
)