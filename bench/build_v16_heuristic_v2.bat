@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

if exist flecs_v16_local.obj del flecs_v16_local.obj
if exist flecs_v16_local.lib del flecs_v16_local.lib

cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c flecs_patched_v16.c /Fo:flecs_v16_local.obj
if errorlevel 1 exit /b 1
lib /OUT:flecs_v16_local.lib flecs_v16_local.obj
if errorlevel 1 exit /b 1

if exist test_auto_dont_fragment_lazy.exe del test_auto_dont_fragment_lazy.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include test_auto_dont_fragment_lazy.c /Fe:test_auto_dont_fragment_lazy.exe flecs_v16_local.lib
if errorlevel 1 exit /b 1

"C:\Project\Flecs\bench\test_auto_dont_fragment_lazy.exe" 2>&1 | head -30