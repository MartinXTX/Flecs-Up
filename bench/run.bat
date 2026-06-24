@echo off
cd /d "C:\Project\Flecs\bench"
echo === Running baseline benchmark, n=100 ===
".\bench_flecs.exe" 100
echo === Running baseline benchmark, n=1000 ===
".\bench_flecs.exe" 1000
echo === Exit: %errorlevel% ===
