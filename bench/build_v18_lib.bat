@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
echo === Compiling flecs_patched_v18.c ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c flecs_patched_v18.c /Fo:flecs_v18.obj 2>&1
if errorlevel 1 (
    echo === COMPILE FAILED ===
    exit /b 1
)
if exist release\v18_flecs_patched.lib del release\v18_flecs_patched.lib
lib /OUT:release\v18_flecs_patched.lib flecs_v18.obj
if errorlevel 1 (
    echo === LIB FAILED ===
    exit /b 1
)
echo === v18 lib OK ===
dir release\v18_flecs_patched.lib
