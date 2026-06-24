@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Build for v17
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include bench_dm_stress.c /Fe:bench_dm_stress_v17.exe release\v17_flecs_patched.lib 1>nul
echo v17 build: %ERRORLEVEL%

REM Build for v16
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include bench_dm_stress.c /Fe:bench_dm_stress_v16.exe release\v16_flecs_patched.lib 1>nul
echo v16 build: %ERRORLEVEL%

echo === v17 run ===
.\bench_dm_stress_v17.exe 1000 100

echo === v16 run ===
.\bench_dm_stress_v16.exe 1000 100