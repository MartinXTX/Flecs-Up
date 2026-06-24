@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Build local flecs_patched.lib matching Tier-v14.1 EXE flags
echo === Building flecs_patched.lib (/O2 /Zi /MDd) ===
cl /c /O2 /Zi /MDd /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . flecs_patched.c /Fo:flecs_local.obj
lib /OUT:flecs_local.lib flecs_local.obj
echo === lib exit: %errorlevel% ===

REM Build test against local lib
echo === Building test_ecs_data_component ===
cl /O2 /Zi /MDd /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . test_init_only.c flecs_local.lib /Fe:test_init_only.exe
echo === compile exit: %errorlevel% ===

"C:\Project\Flecs\bench\test_init_only.exe" > test_init_only_out.txt 2>&1
type test_init_only_out.txt