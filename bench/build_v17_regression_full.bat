@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Build v17 lib (Tier-v14.1 + P2-2 + P2-3 + P0-3 dense_map)
if not exist release\v17_flecs_patched.lib (
  if not exist flecs_v17.obj cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c flecs_patched_v17.c /Fo:flecs_v17.obj
  lib /OUT:release\v17_flecs_patched.lib flecs_v17.obj
)

set RC=0

REM Build + run each Tier-v14.1 5-suite test against v17 lib
for %%T in (test_correctness test_stress test_observer test_edge test_stability) do (
  echo === Building %%T against v17 lib ===
  cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include %%T.c /Fe:%%T_v17.exe release\v17_flecs_patched.lib 1>nul
  if errorlevel 1 (
    echo %%T build FAILED
    set /a RC+=1
  ) else (
    echo === Running %%T ===
    "C:\Project\Flecs\bench\%%T_v17.exe" > %%T_v17_out.txt 2>&1
    findstr /B /C:"PASS" /C:"FAIL" /C:"Total" %%T_v17_out.txt | findstr /V "MSVC"
  )
)

echo === v17 regression complete (RC=%RC%) ===
exit /b %RC%