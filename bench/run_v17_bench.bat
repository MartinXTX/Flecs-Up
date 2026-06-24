@echo off
cd /d "C:\Project\Flecs\bench"
release\bench_dense_map_v17.exe 1000000 42
echo --- EXIT: %ERRORLEVEL% ---