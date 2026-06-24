@echo off
cd /d "C:\Project\Flecs\distr"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . flecs.c /Fo:flecs_test_v14.obj 2>&1 | findstr /C:"error" /C:"warning C4"
echo === exit: %errorlevel% ===