@echo off
REM PGO build pipeline for MSVC
REM Step 1: instrumented build (gen profile)
REM Step 2: run with representative workload
REM Step 3: optimized build (use profile)
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
  echo VCVARS FAILED
  exit /b 1
)

REM Step 1: PGO instrumented build
echo === Step 1: PGO instrumented build ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /GL /Gw /I . ^
   /GenPgoInstrumentation /FASTGENPROFILEPGO ^
   bench_flecs.c flecs_patched.c /Fe:bench_pgo_gen.exe
if errorlevel 1 exit /b 1
echo === exit: %errorlevel% ===

REM Step 2: run with n=200 to collect profile
echo === Step 2: collect profile data ===
.\bench_pgo_gen.exe 200 > nul 2>&1
echo === exit: %errorlevel% ===

REM Step 3: PGO optimized build
echo === Step 3: PGO optimized build ===
move /y pgogen.bin pgo_train.bin 2>nul
move /y bench_pgo_gen!*.pgc pgo_train.pgc 2>nul

cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /GL /Gw /I . ^
   /USEPGOPGO /PGO:pgo_train.pgc ^
   bench_flecs.c flecs_patched.c /Fe:bench_pgo.exe
if errorlevel 1 exit /b 1
echo === exit: %errorlevel% ===

REM Run optimized
echo === Run PGO-optimized, n=200 ===
.\bench_pgo.exe 200
echo === exit: %errorlevel% ===
