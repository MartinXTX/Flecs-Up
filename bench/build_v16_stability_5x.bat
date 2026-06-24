@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM v16 stability 5x rerun
for /L %%i in (1,1,5) do (
  echo === Stability run %%i ===
  "C:\Project\Flecs\bench\test_stability_v16.exe" > stab_v16_%%i_out.txt 2>&1
  findstr /C:"ALL" /C:"Total" stab_v16_%%i_out.txt
)