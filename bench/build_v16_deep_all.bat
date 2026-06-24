@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Build v16 lib if needed
if not exist release\v16_flecs_patched.lib (
  if not exist flecs_v16.obj cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v16.c /Fo:flecs_v16.obj
  lib /OUT:release\v16_flecs_patched.lib flecs_v16.obj
)

set RC=0
set TOTAL=0
set PASSED=0

REM Build + run all Tier-v14.1 deep tests against v16
for %%T in (v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v15_leak) do (
  set /a TOTAL+=1
  echo === Building test_v14_1_deep_%%T ===
  if exist test_v14_1_deep_%%T_v16.exe del test_v14_1_deep_%%T_v16.exe 2>nul
  cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_v14_1_deep_%%T.c /Fe:test_v14_1_deep_%%T_v16.exe release\v16_flecs_patched.lib 1>nul 2>nul
  if errorlevel 1 (
    echo BUILD FAILED for %%T
    set /a RC+=1
  ) else (
    "C:\Project\Flecs\bench\test_v14_1_deep_%%T_v16.exe" > test_v14_1_deep_%%T_v16_out.txt 2>&1
    findstr /B /C:"Total" /C:"Passed" /C:"Failed" test_v14_1_deep_%%T_v16_out.txt >nul
    if !errorlevel! equ 0 (
      findstr /C:"Failed: 0" test_v14_1_deep_%%T_v16_out.txt >nul
      if !errorlevel! equ 0 (
        echo %%T: PASS
        set /a PASSED+=1
      ) else (
        echo %%T: FAIL
        set /a RC+=1
      )
    ) else (
      findstr /C:"ALL" /C:"PASSED" test_v14_1_deep_%%T_v16_out.txt >nul
      if !errorlevel! equ 0 (
        echo %%T: PASS
        set /a PASSED+=1
      ) else (
        echo %%T: UNKNOWN - check out file
      )
    )
  )
)

echo === Summary: %PASSED%/%TOTAL% passed, RC=%RC% ===
exit /b %RC%