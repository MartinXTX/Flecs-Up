@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Build v17 dense_map lib (already built earlier; idempotent)
if not exist release\v17_flecs_patched.lib (
  if not exist flecs_v17.obj cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c flecs_patched_v17.c /Fo:flecs_v17.obj
  lib /OUT:release\v17_flecs_patched.lib flecs_v17.obj
)

REM Compile + run bench
if exist release\bench_dense_map_v17.exe del release\bench_dense_map_v17.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include bench_dense_map.c /Fe:release\bench_dense_map_v17.exe release\v17_flecs_patched.lib 1>nul
if errorlevel 1 (
  echo === bench_dense_map build FAILED ===
  exit /b 1
)
echo === Running v17 dense_map bench (1M entities, 1M get_id) ===
release\bench_dense_map_v17.exe 1000000 42