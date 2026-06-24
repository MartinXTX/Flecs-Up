@echo off
REM Final comparison: baseline /O2 vs patched /O2 (no LTCG) — Tier-A1 + Tier-C2 + Tier-C1 + Tier-A2
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
  echo VCVARS FAILED
  exit /b 1
)

echo === Build BASELINE /O2 ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I ../distr bench_flecs.c ../distr/flecs_no_addons.c /Fe:bench_baseline.exe
if errorlevel 1 exit /b 1

echo === Build PATCHED /O2 (Tüm Tier patch'ler) ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . bench_flecs.c flecs_patched.c /Fe:bench_patched.exe
if errorlevel 1 exit /b 1

echo === Run BASELINE /O2 ===
.\bench_baseline.exe 1000 2>&1
echo.
echo === Run PATCHED /O2 ===
.\bench_patched.exe 1000 2>&1
