@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" 1>nul 2>&1
if errorlevel 1 (
    echo === VCVARS FAILED ===
    exit /b 1
)
cd /d "C:\Project\Flecs\bench"

echo.
echo ==================================================================
echo === Tier-v19 Full Test Suite ===
echo ==================================================================

REM Verify v19 lib exists
if not exist release\v19_flecs_patched.lib (
    echo === v19 lib missing — run build_v19_quick.bat first ===
    exit /b 1
)

REM Build benchmark exe (regression test)
echo === Building bench_flecs.exe (5-suite regression) ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v19.c /Fo:flecs_v19.obj 2>&1 | findstr /V "^flecs_patched" | findstr /V "^Microsoft" | findstr /V "^Copyright"
if errorlevel 1 (
    echo === COMPILE FAILED ===
    exit /b 1
)
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include bench_flecs.c /Fe:release\bench_flecs.exe flecs_v19.obj
echo === bench_flecs.exe built ===

REM Build + run per-Tier tests
set FAILED=0

REM BULGU-41
if exist test_bulgu_41_pair.c (
    echo.
    echo === Building BULGU-41 test ===
    cl /O2 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_bulgu_41_pair.c /Fe:release\test_bulgu_41_pair.exe flecs_v19.obj 2>&1 | findstr /V "^Microsoft" | findstr /V "^Copyright"
    if exist release\test_bulgu_41_pair.exe (
        release\test_bulgu_41_pair.exe
        if errorlevel 1 set /a FAILED+=1
    ) else (
        set /a FAILED+=1
    )
)

REM P2-1 entity-index flat
if exist test_entity_index_flat.c (
    echo.
    echo === Building P2-1 entity-index flat test ===
    cl /O2 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_entity_index_flat.c /Fe:release\test_entity_index_flat.exe flecs_v19.obj 2>&1 | findstr /V "^Microsoft" | findstr /V "^Copyright"
    if exist release\test_entity_index_flat.exe (
        release\test_entity_index_flat.exe
        if errorlevel 1 set /a FAILED+=1
    ) else (
        set /a FAILED+=1
    )
)

REM C1 observer bitmap
if exist test_c1_bitmap.c (
    echo.
    echo === Building C1 observer bitmap test ===
    cl /O2 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_c1_bitmap.c /Fe:release\test_c1_bitmap.exe flecs_v19.obj 2>&1 | findstr /V "^Microsoft" | findstr /V "^Copyright"
    if exist release\test_c1_bitmap.exe (
        release\test_c1_bitmap.exe
        if errorlevel 1 set /a FAILED+=1
    ) else (
        set /a FAILED+=1
    )
)

REM B2 tiny archetype
if exist test_b2_tiny.c (
    echo.
    echo === Building B2 tiny archetype test ===
    cl /O2 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_b2_tiny.c /Fe:release\test_b2_tiny.exe flecs_v19.obj 2>&1 | findstr /V "^Microsoft" | findstr /V "^Copyright"
    if exist release\test_b2_tiny.exe (
        release\test_b2_tiny.exe
        if errorlevel 1 set /a FAILED+=1
    ) else (
        set /a FAILED+=1
    )
)

REM A3 arena
if exist test_a3_arena.c (
    echo.
    echo === Building A3 arena test ===
    cl /O2 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_a3_arena.c /Fe:release\test_a3_arena.exe flecs_v19.obj 2>&1 | findstr /V "^Microsoft" | findstr /V "^Copyright"
    if exist release\test_a3_arena.exe (
        release\test_a3_arena.exe
        if errorlevel 1 set /a FAILED+=1
    ) else (
        set /a FAILED+=1
    )
)

REM B3 varid
if exist test_b3_varid.c (
    echo.
    echo === Building B3 variable-size IDs test ===
    if exist test_b3_helpers.c (
        cl /O2 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /DFLECS_STATIC /I . /I ..\include test_b3_varid.c test_b3_helpers.c /Fe:release\test_b3_varid.exe release\v19_flecs_patched.lib 2>&1 | findstr /V "^Microsoft" | findstr /V "^Copyright"
        if exist release\test_b3_varid.exe (
            release\test_b3_varid.exe
            if errorlevel 1 set /a FAILED+=1
        )
    )
)

REM C3 SIMD
if exist test_c3_simd.c (
    echo.
    echo === Building C3 SIMD test (AVX2) ===
    cl /O2 /arch:AVX2 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_c3_simd.c /Fe:release\test_c3_simd.exe flecs_v19.obj 2>&1 | findstr /V "^Microsoft" | findstr /V "^Copyright"
    if exist release\test_c3_simd.exe (
        release\test_c3_simd.exe
        if errorlevel 1 set /a FAILED+=1
    ) else (
        set /a FAILED+=1
    )
)

REM E2+E3
if exist test_e2_e3.c (
    echo.
    echo === Building E2+E3 hardware test ===
    cl /O2 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_e2_e3.c /Fe:release\test_e2_e3.exe flecs_v19.obj 2>&1 | findstr /V "^Microsoft" | findstr /V "^Copyright"
    if exist release\test_e2_e3.exe (
        release\test_e2_e3.exe
        if errorlevel 1 set /a FAILED+=1
    ) else (
        set /a FAILED+=1
    )
)

REM Tier-A1 sparse hybrid (existing test)
if exist test_sparse_hybrid.c (
    echo.
    echo === Building Tier-A1 sparse hybrid test ===
    cl /O2 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_sparse_hybrid.c /Fe:release\test_sparse_hybrid.exe flecs_v19.obj 2>&1 | findstr /V "^Microsoft" | findstr /V "^Copyright"
    if exist release\test_sparse_hybrid.exe (
        release\test_sparse_hybrid.exe
        if errorlevel 1 set /a FAILED+=1
    ) else (
        set /a FAILED+=1
    )
)

REM LAZY heuristic
if exist test_auto_dont_fragment_lazy.c (
    echo.
    echo === Building LAZY heuristic test ===
    cl /O2 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_auto_dont_fragment_lazy.c /Fe:release\test_lazy.exe flecs_v19.obj 2>&1 | findstr /V "^Microsoft" | findstr /V "^Copyright"
    if exist release\test_lazy.exe (
        release\test_lazy.exe
        if errorlevel 1 set /a FAILED+=1
    ) else (
        set /a FAILED+=1
    )
)

echo.
echo ==================================================================
echo === Tier-v19 Test Suite Complete ===
echo === FAILED=%FAILED% ===
echo ==================================================================
exit /b %FAILED%