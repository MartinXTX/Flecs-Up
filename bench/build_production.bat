@echo off
REM Production build: compile flecs_patched.c as a static library + bench.
REM Tier patches aktif, LTCG yok, /O2.
REM Output: release\flecs_patched.lib, release\bench_flecs.exe
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
  echo VCVARS FAILED
  exit /b 1
)

if not exist release mkdir release

echo === Build STATIC LIBRARY (flecs_patched.lib) ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD ^
   /I . /I ../include /c flecs_patched.c /Fo:release\flecs_patched.obj
if errorlevel 1 exit /b 1
lib /OUT:release\flecs_patched.lib release\flecs_patched.obj
if errorlevel 1 exit /b 1

echo === Build BENCHMARK ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD ^
   /I . /I ../include bench_flecs.c /Fe:release\bench_flecs.exe ^
   release\flecs_patched.lib
if errorlevel 1 exit /b 1

echo.
echo === Production build OK ===
echo Library: release\flecs_patched.lib
echo Benchmark: release\bench_flecs.exe
echo.
echo Run: release\bench_flecs.exe 1000