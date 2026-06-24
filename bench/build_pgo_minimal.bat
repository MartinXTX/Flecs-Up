@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" 1>nul 2>&1
cd /d "C:\Project\Flecs\bench"
del /Q test_pgo_minimal.obj test_pgo_minimal.pgd test_pgo_minimal.pgc 2>nul
cl /O2 /GL test_pgo_minimal.c /Fe:test_pgo_minimal_instr.exe /link /LTCG:PGINSTRUMENT
if errorlevel 1 exit /b 1
dir test_pgo_minimal*
echo === Running instrumented binary ===
test_pgo_minimal_instr.exe
dir test_pgo_minimal*