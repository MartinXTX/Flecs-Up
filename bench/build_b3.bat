@echo off
REM TIER-B3 framework build script (uses Tier-v19 lib)
REM
REM Strategy: link against Tier-v19 lib (DFLECS_STATIC) which has
REM flecs_id_size_bits/flecs_record_size already exported.

cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

if not exist release mkdir release

REM Confirm Tier-v19 lib exists
if not exist release\v19_flecs_patched.lib (
    echo === ERROR: v19_flecs_patched.lib not found in release\.  ===
    echo ===   Run build_v19_quick.bat first.    ===
    exit /b 1
)

echo === Compiling test_b3_varid.c (size reporters in v19 lib) ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /DFLECS_STATIC /I . /I ..\include test_b3_varid.c /Fe:release\test_b3_varid.exe release\v19_flecs_patched.lib 2>&1
if errorlevel 1 (
    echo === TEST COMPILE FAILED ===
    exit /b 1
)

echo === Running test_b3_varid.exe ===
release\test_b3_varid.exe
exit /b %errorlevel%
