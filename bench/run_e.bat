@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1

echo === Build E-only baseline ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I ../distr bench_e.c ../distr/flecs_no_addons.c /Fe:bench_e_baseline.exe 2>&1 | findstr /v "^Microsoft\|^Copyright\|^/out:\|Telif"
echo === Build E-only patched ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I . bench_e_patched.c flecs_patched.c /Fe:bench_e_patched.exe 2>&1 | findstr /v "^Microsoft\|^Copyright\|^/out:\|Telif"

echo.
echo === Run E-only BASELINE, n=500 ===
".\bench_e_baseline.exe" 500
echo === exit: %errorlevel% ===
echo.
echo === Run E-only PATCHED, n=500 ===
".\bench_e_patched.exe" 500
echo === exit: %errorlevel% ===
