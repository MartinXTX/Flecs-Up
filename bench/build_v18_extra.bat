@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
set RC=0

for %%T in (test_a13_only test_auto_heuristic test_ecs_data_component test_auto_dont_fragment_lazy) do (
  if exist %%T_v18.exe del %%T_v18.exe
  echo === Building %%T against v18 lib ===
  cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include %%T.c /Fe:%%T_v18.exe release\v18_flecs_patched.lib 1>nul
  if errorlevel 1 (
    echo %%T build FAILED
    set /a RC+=1
  ) else (
    echo === Running %%T ===
    .\%%T_v18.exe
    echo Exit: %ERRORLEVEL%
  )
)

echo === v18 extra tests complete (RC=%RC%) ===
exit /b %RC%
