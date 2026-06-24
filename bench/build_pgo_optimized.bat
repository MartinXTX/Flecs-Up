@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" 1>nul 2>&1
if errorlevel 1 (
    echo === VCVARS FAILED ===
    exit /b 1
)
cd /d C:\Project\Flecs\bench
echo === PGO Optimized Build ===

REM Combine all .pgc files (each process creates a new one)
if exist release\v18_pgo_flecs_patched.lib del release\v18_pgo_flecs_patched.lib
if exist release\v18_pgo_flecs_patched_bench.exe del release\v18_pgo_flecs_patched_bench.exe
del /Q flecs_v18_pgo_opt.obj 2>nul

REM Step 1: compile flecs source WITH /GL for LTCG + PGO optimized
cl /O2 /GL /arch:AVX2 /DFLECS_PATCHED_BUILD /DFLECS_STATIC ^
   /I . /I ..\include /c flecs_patched_v18.c /Fo:flecs_v18_pgo_opt.obj
if errorlevel 1 (
    echo === COMPILE FAILED ===
    exit /b 1
)

REM Step 2: compile bench WITH /GL
cl /O2 /GL /arch:AVX2 /DFLECS_PATCHED_BUILD /DFLECS_STATIC ^
   /I . /I ..\include /c bench_flecs.c /Fo:flecs_pgo_opt_bench.obj
if errorlevel 1 (
    echo === BENCH COMPILE FAILED ===
    exit /b 1
)

REM Step 3: PGO link — /USEPROFILE consumes .pgd + .pgc from .exe path
copy /Y release\v18_pgo_instr_bench.pgd release\v18_pgo_flecs_patched_bench.pgd >nul
REM Remove any stale .pgc for optimized exe (they are for instr exe)
del /Q release\v18_pgo_flecs_patched_bench!*.pgc 2>nul
link /LTCG /OUT:release\v18_pgo_flecs_patched_bench.exe ^
      flecs_v18_pgo_opt.obj flecs_pgo_opt_bench.obj
if errorlevel 1 (
    echo === LINK FAILED ===
    exit /b 1
)
if errorlevel 1 (
    echo === LINK FAILED ===
    exit /b 1
)

REM Step 4: extract .lib from the optimized exe for downstream use
lib /OUT:release\v18_pgo_flecs_patched.lib flecs_v18_pgo_opt.obj
if errorlevel 1 (
    echo === LIB FAILED ===
    exit /b 1
)

dir release\v18_pgo_flecs_patched*
echo === PGO optimized build OK ===
exit /b 0