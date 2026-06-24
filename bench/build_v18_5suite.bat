@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
set RC=0

REM Build + run each Tier-v14.1 5-suite test against v18 lib
for %%T in (test_correctness test_stress test_observer test_edge test_stability) do (
  if exist %%T_v18.exe del %%T_v18.exe
  echo === Building %%T against v18 lib ===
  cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include %%T.c /Fe:%%T_v18.exe release\v18_flecs_patched.lib 1>nul
  if errorlevel 1 (
    echo %%T build FAILED
    set /a RC+=1
  ) else (
    echo === Running %%T ===
    "%%T_v18.exe" > %%T_v18_out.txt 2>&1
    echo Exit: %ERRORLEVEL%
  )
)

echo === v18 5-suite complete (RC=%RC%) ===
exit /b %RC%
