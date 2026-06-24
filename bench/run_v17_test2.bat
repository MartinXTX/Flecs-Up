@echo off
cd /d "C:\Project\Flecs\bench"
echo Running from: %CD%
dir test_correctness_v17.exe
test_correctness_v17.exe
echo --- EXIT: %ERRORLEVEL% ---