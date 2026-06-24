@echo off
REM build_b1.bat — Build Tier-B1 patched library and test.
REM
REM Tier-B1 = hot/cold split for ecs_record_t.
REM Real wins (vs. full struct rewrite that breaks ABI):
REM   - Re-enable single-slot sequential cache in flecs_entity_index_try_get_any
REM     (was DISABLED with "TIER-FF1 DISABLED" comment in v18).
REM   - Add flecs_entity_index_prefetch() L1 hint for batch lookups.
REM
REM Build:
REM   cd C:\Project\Flecs\bench
REM   build_b1.bat
REM
REM Outputs:
REM   bench\release\v18_b1_flecs_patched.lib
REM   bench\release\test_b1_hotcold.exe

setlocal

cd /d "C:\Project\Flecs\bench"
if not exist release mkdir release

call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo === vcvars64.bat failed ===
    exit /b 1
)

echo === Compiling flecs_patched_v18.c - Tier-B1, cache ENABLED ===
if exist release\flecs_v18_b1.obj del release\flecs_v18_b1.obj
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c flecs_patched_v18.c /Fo:release\flecs_v18_b1.obj
if errorlevel 1 (
    echo === COMPILE B1 FAILED ===
    exit /b 1
)

echo === Creating B1 library ===
if exist release\v18_b1_flecs_patched.lib del release\v18_b1_flecs_patched.lib
lib /OUT:release\v18_b1_flecs_patched.lib release\flecs_v18_b1.obj
if errorlevel 1 (
    echo === LIB B1 FAILED ===
    exit /b 1
)

echo === Compiling flecs_patched_v18.c - BASELINE, cache DISABLED ===
if exist release\flecs_v18_b1_baseline.obj del release\flecs_v18_b1_baseline.obj
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /DFLECS_B1_DISABLE_CACHE /I . /I ../include /c flecs_patched_v18.c /Fo:release\flecs_v18_b1_baseline.obj
if errorlevel 1 (
    echo === COMPILE BASELINE FAILED ===
    exit /b 1
)

echo === Creating BASELINE library ===
if exist release\v18_b1_baseline_flecs_patched.lib del release\v18_b1_baseline_flecs_patched.lib
lib /OUT:release\v18_b1_baseline_flecs_patched.lib release\flecs_v18_b1_baseline.obj
if errorlevel 1 (
    echo === LIB BASELINE FAILED ===
    exit /b 1
)

echo === Compiling test_b1_hotcold.c - B1 build ===
if exist release\test_b1_hotcold_b1.exe del release\test_b1_hotcold_b1.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include test_b1_hotcold.c /Fe:release\test_b1_hotcold_b1.exe release\v18_b1_flecs_patched.lib
if errorlevel 1 (
    echo === TEST COMPILE B1 FAILED ===
    exit /b 1
)

echo === Compiling test_b1_hotcold.c - BASELINE build ===
if exist release\test_b1_hotcold_baseline.exe del release\test_b1_hotcold_baseline.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /DFLECS_B1_DISABLE_CACHE /I . /I ../include test_b1_hotcold.c /Fe:release\test_b1_hotcold_baseline.exe release\v18_b1_baseline_flecs_patched.lib
if errorlevel 1 (
    echo === TEST COMPILE BASELINE FAILED ===
    exit /b 1
)

echo === Running B1 test - cache ENABLED ===
release\test_b1_hotcold_b1.exe
echo RC=%ERRORLEVEL%

echo === Running B1 test - cache DISABLED baseline ===
release\test_b1_hotcold_baseline.exe
echo RC=%ERRORLEVEL%

endlocal
