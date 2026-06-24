@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1
if errorlevel 1 (
  echo VCVARS FAILED
  exit /b 1
)
echo === cl invoke ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I ../distr bench_flecs.c ../distr/flecs_no_addons.c /Fe:bench_flecs.exe
echo === exit code: %errorlevel% ===
