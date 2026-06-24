@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" 1>nul 2>&1
if errorlevel 1 (
    echo === VCVARS FAILED ===
    exit /b 1
)
cd /d "C:\Project\Flecs\bench"
echo === Compiling flecs_patched_v19.c (static lib build) ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /DFLECS_STATIC /I . /I ..\include /c flecs_patched_v19.c /Fo:flecs_v19.obj 2>&1
if errorlevel 1 (
    echo === COMPILE FAILED ===
    exit /b 1
)
if exist release\v19_flecs_patched.lib del release\v19_flecs_patched.lib
lib /OUT:release\v19_flecs_patched.lib flecs_v19.obj
if errorlevel 1 (
    echo === LIB FAILED ===
    exit /b 1
)
echo === Tier-v19 lib OK ===
dir release\v19_flecs_patched.lib
exit /b 0