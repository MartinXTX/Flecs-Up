@echo off
cd /d C:\Project\Flecs\bench
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >NUL 2>&1
cd /d C:\Project\Flecs\bench\release
del /q a3_v18a3.log a3_v18.log 2>nul
echo --- build v18 baseline test ---
cd /d C:\Project\Flecs\bench
cl /nologo /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_a3_arena.c /Fe:release\test_a3_v18.exe release\v18_flecs_patched.lib 1>nul 2>&1
cd /d C:\Project\Flecs\bench\release
echo --- v18_a3 (with TIER-A3) 10000 iters ---
".\test_a3_arena.exe" 10000 10000 3 > a3_v18a3.log 2>&1
echo --- exit=%errorlevel% ---
type a3_v18a3.log
echo --- v18 (baseline) 10000 iters ---
".\test_a3_v18.exe" 10000 10000 3 > a3_v18.log 2>&1
echo --- exit=%errorlevel% ---
type a3_v18.log
exit /b 0