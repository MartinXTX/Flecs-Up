@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

if exist flecs_v16_local.obj del flecs_v16_local.obj
if exist flecs_v16_local.lib del flecs_v16_local.lib

echo === Build flecs_v16_local.lib from flecs_patched_v16.c ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c flecs_patched_v16.c /Fo:flecs_v16_local.obj
if errorlevel 1 exit /b 1
lib /OUT:flecs_v16_local.lib flecs_v16_local.obj
if errorlevel 1 exit /b 1

if exist test_ecs_data_component.exe del test_ecs_data_component.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include test_ecs_data_component.c /Fe:test_ecs_data_component.exe flecs_v16_local.lib
if errorlevel 1 exit /b 1

"C:\Project\Flecs\bench\test_ecs_data_component.exe" > test_ecs_data_component_v16_out.txt 2>&1
type test_ecs_data_component_v16_out.txt