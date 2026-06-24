@echo off
cd /d "C:\Project\Flecs\bench"
echo === Tier-v18 P1-3 (lazy override) benchmark: v17 vs v18 ===
echo.
echo === N=1000, ITERS=5000 ===
.\bench_override_v17.exe 1000 5000
echo.
.\bench_override_v18.exe 1000 5000
echo.
echo === N=5000, ITERS=1000 ===
.\bench_override_v17.exe 5000 1000
echo.
.\bench_override_v18.exe 5000 1000
echo.
echo === N=10000, ITERS=500 ===
.\bench_override_v17.exe 10000 500
echo.
.\bench_override_v18.exe 10000 500
