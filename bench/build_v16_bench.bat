@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Tier-v16 build (Tier-v14.1 + BULGU-08 v2 + TIER-SI1 retry)
if exist flecs_v16.obj del flecs_v16.obj
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c flecs_patched_v16.c /Fo:flecs_v16.obj
if errorlevel 1 exit /b 1

if exist release\v16_bench_flecs.exe del release\v16_bench_flecs.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include bench_flecs.c /Fe:release\v16_bench_flecs.exe flecs_v16.obj
if errorlevel 1 exit /b 1

echo === v16 build OK ===
echo Running v16 benchmark with 200 iters...
release\v16_bench_flecs.exe 200 > v16_bench_out.txt 2>&1
type v16_bench_out.txt