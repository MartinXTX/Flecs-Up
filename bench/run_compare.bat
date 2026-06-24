@echo off
REM Compare v18_a3 (with TIER-A3) vs v17 baseline
cd /d C:\Project\Flecs\bench\release
del /q a3_v18a3.log a3_v17.log 2>nul
echo === v18_a3 (with TIER-A3) ===
".\test_a3_arena.exe" %* > a3_v18a3.log 2>&1
echo exit=%errorlevel%
type a3_v18a3.log | findstr /R "Time: Phase Checksums"
echo.
echo === v17 (baseline, no TIER-A3) ===
".\test_a3_v17.exe" %* > a3_v17.log 2>&1
echo exit=%errorlevel%
type a3_v17.log | findstr /R "Time: Phase Checksums"
exit /b 0