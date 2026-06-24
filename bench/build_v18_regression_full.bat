@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >/dev/null 2>&1

if not exist release\v18_flecs_patched.lib (
  cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v18.c /Fo:flecs_v18.obj
  if errorlevel 1 exit /b 1
  lib /OUT:release\v18_flecs_patched.lib flecs_v18.obj
  if errorlevel 1 exit /b 1
)

set RC=0
for %%T in (test_correctness test_stress test_observer test_edge test_stability) do (
  echo === Building %%T against v18 lib ===
  cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include %%T.c /Fe:%%T_v18.exe release\v18_flecs_patched.lib 1>/dev/null
  if errorlevel 1 (
    echo %%T build FAILED
    set /a RC+=1
  ) else (
    echo === Running %%T ===
    "C:\Project\Flecs\bench\%%T_v18.exe" > %%T_v18_out.txt 2>&1
    findstr /B /C:"PASS" /C:"FAIL" /C:"Total" %%T_v18_out.txt | findstr /V "MSVC"
  )
)

echo === v18 regression complete (RC=%RC%) ===
exit /b %RC%
