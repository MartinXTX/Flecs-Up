@echo off
cd /d "C:\Project\Flecs\bench"
release\bench_dense_map_v17.exe 100 42
echo --- EXIT: %ERRORLEVEL% ---