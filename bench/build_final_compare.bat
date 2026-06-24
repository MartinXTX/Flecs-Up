@echo off
REM Final comparison: baseline vs full-patched (Tier-C2 + Tier-C1 + Tier-D2 + Tier-A2)
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
  echo VCVARS FAILED
  exit /b 1
)

echo === Build BASELINE ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I ../distr bench_flecs.c ../distr/flecs_no_addons.c /Fe:bench_baseline.exe
if errorlevel 1 exit /b 1

echo === Build PATCHED with Tier-C2 (prefetch) + Tier-C1 (hooks) + Tier-A2 (unchecked field) ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /GL /Gw /DFLECS_PATCHED_BUILD /I . bench_flecs.c flecs_patched.c /Fe:bench_ltcg.exe /link /LTCG
if errorlevel 1 exit /b 1

echo === BASELINE /O2 ===
.\bench_baseline.exe 1000 2>&1
echo.
echo === PATCHED /O2 + /GL + /LTCG (Tier-C2 + C1 + A2) ===
.\bench_ltcg.exe 1000 2>&1
