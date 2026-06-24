@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Build v16 production lib
if exist flecs_v16.lib del flecs_v16.lib
if exist release\v16_flecs_patched.lib del release\v16_flecs_patched.lib
lib /OUT:release\v16_flecs_patched.lib flecs_v16.obj
if errorlevel 1 exit /b 1
echo === v16 production lib built ===

REM Rebuild v15 leak test (Tier-v14.1 base) with v16 lib for regression
echo === Building test_v14_1_deep_v15_leak against v16 ===
if exist test_v14_1_deep_v15_leak.exe del test_v14_1_deep_v15_leak.exe
cl /O2 /Zi /MDd /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include test_v14_1_deep_v15_leak.c /Fe:test_v14_1_deep_v15_leak_v16.exe release\v16_flecs_patched.lib
if errorlevel 1 exit /b 1
echo === leak test build OK ===

REM Run the leak test
"C:\Project\Flecs\bench\test_v14_1_deep_v15_leak_v16.exe" > test_v14_1_deep_v15_leak_v16_out.txt 2>&1
type test_v14_1_deep_v15_leak_v16_out.txt