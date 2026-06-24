@echo off
cd /d "C:\Project\Flecs\bench"
test_correctness_v17.exe
echo --- exit code: %ERRORLEVEL% ---