@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
REM Build a clean copy of v17 as test_clean_v17.obj (no patches at all)
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v17.c /Fo:test_clean_v17.obj
if errorlevel 1 exit /b 1
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include diag_nopatch.c /Fe:diag_nopatch_clean.exe test_clean_v17.obj
if errorlevel 1 exit /b 1
echo === running diag_nopatch with clean v17 ===
diag_nopatch_clean.exe
echo === exit=%errorlevel% ===
del test_clean_v17.obj