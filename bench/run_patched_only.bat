@echo off
cd /d "C:\Project\Flecs\bench"
echo === Run PATCHED alone, n=100 ===
".\bench_patched.exe" 100
echo === exit: %errorlevel% ===
