@echo off
cd /d "C:\Project\Flecs\bench"
echo === Tier-v19 Full Regression Test Suite ===
echo === 5-suite + Tier-A hybrid + BULGU-41 + P2-1 ===

REM 1. Build v19 lib (with all Tier-v18 + Tier-v19 patches)
call build_v19_lib.bat
if errorlevel 1 exit /b 1

REM 2. Build main benchmark
echo === Building bench_flecs.exe ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include bench_flecs.c /Fe:release\bench_flecs.exe flecs_v19.obj
if errorlevel 1 (
    echo === BENCH BUILD FAILED ===
    exit /b 1
)

REM 3. Run main benchmark (regression suite)
echo === Running main benchmark ===
release\bench_flecs.exe
if errorlevel 1 (
    echo === BENCH FAILED ===
    exit /b 1
)

REM 4. Build + run per-Tier tests (if exist)
if exist test_bulgu_41_pair.c (
    echo === Building BULGU-41 test ===
    cl /O2 /DFLECS_PATCHED_BUILD /I . /I ../include test_bulgu_41_pair.c /Fe:release\test_bulgu_41_pair.exe flecs_v19.obj
    if not errorlevel 1 release\test_bulgu_41_pair.exe
)

if exist test_entity_index_flat.c (
    echo === Building P2-1 entity-index flat test ===
    cl /O2 /DFLECS_PATCHED_BUILD /I . /I ../include test_entity_index_flat.c /Fe:release\test_entity_index_flat.exe flecs_v19.obj
    if not errorlevel 1 release\test_entity_index_flat.exe
)

if exist test_c1_bitmap.c (
    echo === Building C1 observer bitmap test ===
    cl /O2 /DFLECS_PATCHED_BUILD /I . /I ../include test_c1_bitmap.c /Fe:release\test_c1_bitmap.exe flecs_v19.obj
    if not errorlevel 1 release\test_c1_bitmap.exe
)

if exist test_b2_tiny.c (
    echo === Building B2 tiny archetype test ===
    cl /O2 /DFLECS_PATCHED_BUILD /I . /I ../include test_b2_tiny.c /Fe:release\test_b2_tiny.exe flecs_v19.obj
    if not errorlevel 1 release\test_b2_tiny.exe
)

if exist test_a3_arena.c (
    echo === Building A3 arena test ===
    cl /O2 /DFLECS_PATCHED_BUILD /I . /I ../include test_a3_arena.c /Fe:release\test_a3_arena.exe flecs_v19.obj
    if not errorlevel 1 release\test_a3_arena.exe
)

if exist test_c3_simd.c (
    echo === Building C3 SIMD test ===
    cl /O2 /arch:AVX2 /DFLECS_PATCHED_BUILD /I . /I ../include test_c3_simd.c /Fe:release\test_c3_simd.exe flecs_v19.obj
    if not errorlevel 1 release\test_c3_simd.exe
)

if exist test_e2_e3.c (
    echo === Building E2/E3 test ===
    cl /O2 /DFLECS_PATCHED_BUILD /I . /I ../include test_e2_e3.c /Fe:release\test_e2_e3.exe flecs_v19.obj
    if not errorlevel 1 release\test_e2_e3.exe
)

REM 5. Run Tier-A sparse hybrid test
if exist test_sparse_hybrid.c (
    echo === Running Tier-A1 sparse hybrid test ===
    cl /O2 /DFLECS_PATCHED_BUILD /I . /I ../include test_sparse_hybrid.c /Fe:release\test_sparse_hybrid.exe flecs_v19.obj
    if not errorlevel 1 release\test_sparse_hybrid.exe
)

REM 6. Run LAZY heuristic test
if exist test_auto_dont_fragment_lazy.c (
    echo === Running LAZY heuristic test ===
    cl /O2 /DFLECS_PATCHED_BUILD /I . /I ../include test_auto_dont_fragment_lazy.c /Fe:release\test_lazy.exe flecs_v19.obj
    if not errorlevel 1 release\test_lazy.exe
)

echo === Tier-v19 REGRESSION COMPLETE ===
exit /b 0