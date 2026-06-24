@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Tier-v14.1 PRODUCTION-READY baseline (flecs_patched.c.bak is the v14.1 source)
if not exist flecs_patched_v14_1.c copy /Y flecs_patched.c.bak flecs_patched_v14_1.c >nul
if exist flecs_local.obj del flecs_local.obj
if exist flecs_local.lib del flecs_local.lib

echo === Build flecs_local.lib from Tier-v14.1 ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c flecs_patched_v14_1.c /Fo:flecs_local.obj
if errorlevel 1 exit /b 1
lib /OUT:flecs_local.lib flecs_local.obj
if errorlevel 1 exit /b 1

echo === Build test_ecs_data_component ===
if exist test_ecs_data_component.exe del test_ecs_data_component.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include test_ecs_data_component.c /Fe:test_ecs_data_component.exe flecs_local.lib
if errorlevel 1 exit /b 1

"C:\Project\Flecs\bench\test_ecs_data_component.exe" > test_ecs_data_component_out.txt 2>&1
type test_ecs_data_component_out.txt