@echo off
REM Tier-D2 (LTCG): /GL (whole program opt) + /LTCG (link-time code gen)
REM This is the closest equivalent to PGO available from command-line cl.exe.
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
  echo VCVARS FAILED
  exit /b 1
)

echo === Build BASELINE (master flecs, /O2) ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I ../distr bench_flecs.c ../distr/flecs_no_addons.c /Fe:bench_baseline.exe 2>&1
if errorlevel 1 exit /b 1

echo === Build P0-1 PATCHED (table_cache + Tier-C2 prefetch + Tier-C1 hook bitmap, /O2) ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I . bench_flecs.c flecs_patched.c /Fe:bench_patched.exe 2>&1
if errorlevel 1 exit /b 1

echo === Build LTCG (whole-program opt, /O2 + /GL + /LTCG) ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /GL /Gw /I . bench_flecs.c flecs_patched.c /Fe:bench_ltcg.exe /link /LTCG 2>&1
if errorlevel 1 exit /b 1

echo === Run BASELINE, n=200 ===
".\bench_baseline.exe" 200
echo.
echo === Run PATCHED (/O2), n=200 ===
".\bench_patched.exe" 200
echo.
echo === Run LTCG (/O2 + /GL + /LTCG), n=200 ===
".\bench_ltcg.exe" 200

echo === exit: %errorlevel% ===
