@echo off
setlocal
cd /d C:\Project\Flecs\bench
echo === Running PGO profile workload (5x bench_flecs) ===
for /L %%i in (1,1,5) do (
    v18_pgo_instr_bench.exe 1000
)
echo === Profile data generated: flecs_v18_pgo.pgc ===
exit /b 0