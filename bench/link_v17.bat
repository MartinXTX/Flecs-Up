@echo off
cd /d C:\Project\Flecs\bench
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >NUL 2>&1
cl /nologo /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_a3_arena.c /Fe:release\test_a3_v17.exe release\v17_baseline_flecs_patched.lib 1>nul 2>&1
echo exit=%errorlevel%
dir release\test_a3_v17.exe
exit /b 0