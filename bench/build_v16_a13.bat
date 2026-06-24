@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Force rebuild Tier-v16 lib
if exist flecs_v16_local.obj del flecs_v16_local.obj
if exist flecs_v16_local.lib del flecs_v16_local.lib
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v16.c /Fo:flecs_v16_local.obj
if errorlevel 1 exit /b 1
lib /OUT:flecs_v16_local.lib flecs_v16_local.obj
if errorlevel 1 exit /b 1

REM Build standalone A1.3 test
if exist test_a13_only.exe del test_a13_only.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_a13_only.c /Fe:test_a13_only.exe flecs_v16_local.lib
if errorlevel 1 exit /b 1

echo === Running A1.3 standalone test ===
"C:\Project\Flecs\bench\test_a13_only.exe" 2>&1
echo === Exit %errorlevel% ===