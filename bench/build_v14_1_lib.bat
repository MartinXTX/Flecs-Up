@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Build Tier-v14.1 lib for archive
if not exist flecs_patched_v14_1.c copy /Y flecs_patched.c.bak flecs_patched_v14_1.c >nul
if exist flecs_v14_1.obj del flecs_v14_1.obj
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v14_1.c /Fo:flecs_v14_1.obj
if errorlevel 1 exit /b 1
lib /OUT:release\flecs_patched_v14_1.lib flecs_v14_1.obj
if errorlevel 1 exit /b 1
echo === v14.1 lib built ===
dir release\flecs_patched_v14_1.lib