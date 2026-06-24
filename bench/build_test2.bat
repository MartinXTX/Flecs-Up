@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl /Od /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include diag_min2.c /Fe:diag2_v17p12.exe flecs_v17_p12.obj
if errorlevel 1 exit /b 1
echo === built ===
diag2_v17p12.exe
echo === exit=%errorlevel% ===