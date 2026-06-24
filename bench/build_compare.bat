@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1
if errorlevel 1 (
  echo VCVARS FAILED
  exit /b 1
)

echo === Build BASELINE (master flecs) ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I ../distr bench_flecs.c ../distr/flecs_no_addons.c /Fe:bench_baseline.exe 2>&1
if errorlevel 1 exit /b 1

echo === Build P0-1 PATCHED (table_cache linked list → vec) ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . bench_flecs.c flecs_patched.c /Fe:bench_patched.exe 2>&1
if errorlevel 1 exit /b 1

echo === Run BASELINE, n=200 ===
".\bench_baseline.exe" 200
echo.
echo === Run P0-1 PATCHED, n=200 ===
".\bench_patched.exe" 200

echo === exit: %errorlevel% ===
