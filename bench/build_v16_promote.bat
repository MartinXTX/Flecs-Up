@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Archive Tier-v14.1 EXE
if exist release\bench_flecs.exe (
  if not exist release\bench_flecs_v14_1.exe (
    copy /Y release\bench_flecs.exe release\bench_flecs_v14_1.exe >nul
    echo Archived Tier-v14.1 EXE -> release\bench_flecs_v14_1.exe
  )
)
if exist release\flecs_patched.lib (
  if not exist release\flecs_patched_v14_1.lib (
    copy /Y release\flecs_patched.lib release\flecs_patched_v14_1.lib >nul
    echo Archived Tier-v14.1 LIB -> release\flecs_patched_v14_1.lib
  )
)

REM Promote Tier-v16 to release
if exist release\v16_flecs_patched.lib (
  copy /Y release\v16_flecs_patched.lib release\flecs_patched.lib >nul
  echo Promoted v16 -> release\flecs_patched.lib
)
if exist release\v16_bench_flecs.exe (
  copy /Y release\v16_bench_flecs.exe release\bench_flecs.exe >nul
  echo Promoted v16 EXE -> release\bench_flecs.exe
)

echo === Release now points to Tier-v16 ===
dir release\flecs_patched.lib release\bench_flecs.exe