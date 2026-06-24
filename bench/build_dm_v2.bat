@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include bench_dm_v2.c /Fe:bench_dm_v2_v17.exe release\v17_flecs_patched.lib 1>nul
echo v17 build: %ERRORLEVEL%
.\bench_dm_v2_v17.exe 200 1000000
echo --- v17 EXIT: %ERRORLEVEL% ---

cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include bench_dm_v2.c /Fe:bench_dm_v2_v16.exe release\v16_flecs_patched.lib 1>nul
echo v16 build: %ERRORLEVEL%
.\bench_dm_v2_v16.exe 200 1000000
echo --- v16 EXIT: %ERRORLEVEL% ---