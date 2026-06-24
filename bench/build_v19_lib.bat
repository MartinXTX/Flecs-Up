@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
echo === Compiling flecs_patched_v19.c (Tier-A Unified + BULGU-41 + P2-1 + C2 + C1 + B2 + B1 + A3 + B3 + C3 + D1.2 + E2 + E3) ===
if not exist flecs_patched_v19.c copy flecs_patched_v18.c flecs_patched_v19.c >nul
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c flecs_patched_v19.c /Fo:flecs_v19.obj 2>&1
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
echo === v19 lib OK ===
dir release\v19_flecs_patched.lib