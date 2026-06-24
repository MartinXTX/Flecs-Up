@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
echo === Building test_sparse_hybrid ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /DFLECS_BASIC_SPARSE_SET test_sparse_hybrid.c /Fe:test_sparse_hybrid.exe release\v17_flecs_patched.lib 2>&1
echo Build test_sparse_hybrid: %ERRORLEVEL%
echo.
echo === Building bench_sparse_hybrid ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /DFLECS_BASIC_SPARSE_SET bench_sparse_hybrid.c /Fe:bench_sparse_hybrid.exe release\v17_flecs_patched.lib 2>&1
echo Build bench_sparse_hybrid: %ERRORLEVEL%
