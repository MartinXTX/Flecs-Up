@echo off
REM ============================================================================
REM Tier-C3: SIMD filter (AVX2) build + run.
REM Builds flecs_patched_v18.c into a static lib, then compiles and runs
REM bench/test_c3_simd.c. Both /arch:AVX2 and FLECS_C3_SIMD are set so the
REM SIMD code path is exercised.
REM ============================================================================
setlocal
set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
set "BENCH_DIR=C:\Project\Flecs\bench"
set "LIB_OUT=%BENCH_DIR%\release\v18_c3_flecs_patched.lib"
set "OBJ_OUT=%BENCH_DIR%\release\flecs_v18_c3.obj"
set "EXE_OUT=%BENCH_DIR%\release\test_c3_simd.exe"

cd /d "%BENCH_DIR%"
if exist "%VCVARS%" (
    call "%VCVARS%" >nul 2>&1
) else (
    echo === WARNING: vcvars64.bat not found at %VCVARS%; assuming cl is in PATH ===
)

echo === [1/3] Compiling flecs_patched_v18.c (with /arch:AVX2) ===
cl /nologo /O2 /W3 /arch:AVX2 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD ^
   /I "%BENCH_DIR%" /I "%BENCH_DIR%\..\include" ^
   /c "%BENCH_DIR%\flecs_patched_v18.c" /Fo:"%OBJ_OUT%"
if errorlevel 1 (
    echo === COMPILE FAILED ===
    exit /b 1
)

if exist "%LIB_OUT%" del "%LIB_OUT%"
echo === [2/3] Building static lib ===
lib /NOLOGO /OUT:"%LIB_OUT%" "%OBJ_OUT%"
if errorlevel 1 (
    echo === LIB FAILED ===
    exit /b 1
)

echo === [3/3] Compiling and linking test_c3_simd.c ===
cl /nologo /O2 /W3 /arch:AVX2 /D_CRT_SECURE_NO_WARNINGS ^
   /DFLECS_C3_SIMD /DFLECS_PATCHED_BUILD ^
   /I "%BENCH_DIR%" /I "%BENCH_DIR%\..\include" ^
   "%BENCH_DIR%\test_c3_simd.c" "%LIB_OUT%" /Fe:"%EXE_OUT%"
if errorlevel 1 (
    echo === TEST COMPILE FAILED ===
    exit /b 1
)

echo === Running %EXE_OUT% ===
"%EXE_OUT%"
echo === exit: %errorlevel% ===
exit /b %errorlevel%