@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Build v16 production lib (if not already)
if not exist release\v16_flecs_patched.lib (
  echo Building v16 lib...
  if not exist flecs_v16.obj cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c flecs_patched_v16.c /Fo:flecs_v16.obj
  lib /OUT:release\v16_flecs_patched.lib flecs_v16.obj
)

set RC=0

REM Build + run each Tier-v14.1 5-suite test against v16 lib
for %%T in (test_correctness test_stress test_observer test_edge test_stability) do (
  echo === Building %%T against v16 lib ===
  cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include %%T.c /Fe:%%T_v16.exe release\v16_flecs_patched.lib 1>nul
  if errorlevel 1 (
    echo %%T build FAILED
    set /a RC+=1
  ) else (
    echo === Running %%T ===
    "C:\Project\Flecs\bench\%%T_v16.exe" > %%T_v16_out.txt 2>&1
    findstr /B /C:"PASS" /C:"FAIL" /C:"Total" %%T_v16_out.txt | findstr /V "MSVC"
  )
)

echo === Regression complete (RC=%RC%) ===
exit /b %RC%