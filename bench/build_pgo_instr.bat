@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" 1>nul 2>&1
if errorlevel 1 (
    echo === VCVARS FAILED ===
    exit /b 1
)
cd /d C:\Project\Flecs\bench
echo === PGO Instrumented Build (correct MSVC workflow) ===
if exist release\v18_pgo_instr_bench.exe del release\v18_pgo_instr_bench.exe
if exist release\v18_pgo_instr.pgd del release\v18_pgo_instr.pgd
if exist release\v18_pgo_instr_bench.pgc del release\v18_pgo_instr_bench.pgc
del /Q flecs_v18_pgo.obj flecs_pgo_bench.obj 2>nul

REM Step 1: compile flecs source to obj (with /GL for LTCG)
cl /O2 /GL /arch:AVX2 /favor:INTEL /DFLECS_PATCHED_BUILD /DFLECS_STATIC ^
   /I . /I ..\include /c flecs_patched_v18.c /Fo:flecs_v18_pgo.obj
if errorlevel 1 (
    echo === FLECS COMPILE FAILED ===
    exit /b 1
)

REM Step 2: compile bench to obj
cl /O2 /GL /arch:AVX2 /favor:INTEL /DFLECS_PATCHED_BUILD /DFLECS_STATIC ^
   /I . /I ..\include /c bench_flecs.c /Fo:flecs_pgo_bench.obj
if errorlevel 1 (
    echo === BENCH COMPILE FAILED ===
    exit /b 1
)

REM Step 3: link WITH /LTCG:PGINSTRUMENT (creates .pgd file alongside .exe)
link /LTCG:PGINSTRUMENT /OUT:release\v18_pgo_instr_bench.exe ^
      flecs_v18_pgo.obj flecs_pgo_bench.obj
if errorlevel 1 (
    echo === LINK FAILED ===
    exit /b 1
)

REM Verify .pgd was created
if not exist release\v18_pgo_instr_bench.pgd (
    echo === WARNING: .pgd not found — instrumentation may not be active ===
)
dir release\v18_pgo_instr_bench.*
echo === Instrumented build OK ===
exit /b 0