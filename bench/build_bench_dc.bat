@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Tier-v14.1 baseline
if exist flecs_local.lib del flecs_local.obj flecs_local.lib
if not exist flecs_patched_v14_1.c copy /Y flecs_patched.c.bak flecs_patched_v14_1.c >nul
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c flecs_patched_v14_1.c /Fo:flecs_local.obj
lib /OUT:flecs_local.lib flecs_local.obj
if errorlevel 1 exit /b 1

if exist bench_ecs_data_component.exe del bench_ecs_data_component.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include bench_ecs_data_component.c /Fe:bench_ecs_data_component.exe flecs_local.lib
if errorlevel 1 exit /b 1

bench_ecs_data_component.exe 100 1000