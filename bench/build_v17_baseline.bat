@echo off
setlocal
cd /d C:\Project\Flecs\bench
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >NUL 2>&1
if errorlevel 1 (
    echo [build_v17] vcvars64 failed
    exit /b 1
)

REM 1) Compile v17 baseline (no TIER-A3) to a fresh obj
cl /nologo /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v17.c /Fo:release\flecs_v17_baseline.obj
if errorlevel 1 (
    echo [build_v17] cl compile failed with %errorlevel%
    exit /b 1
)

REM 2) Archive to lib
lib /nologo /OUT:release\v17_baseline_flecs_patched.lib release\flecs_v17_baseline.obj
if errorlevel 1 (
    echo [build_v17] lib failed with %errorlevel%
    exit /b 1
)

echo [build_v17] OK
exit /b 0