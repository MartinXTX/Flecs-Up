@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

if exist flecs_smoke_v14.obj del flecs_smoke_v14.obj
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched.c /Fo:flecs_smoke_v14.obj
if errorlevel 1 exit /b 1
lib /OUT:flecs_smoke_v14.lib flecs_smoke_v14.obj

if exist smoke_v14.exe del smoke_v14.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include smoke_v14.c /Fe:smoke_v14.exe flecs_smoke_v14.lib
if errorlevel 1 exit /b 1

"C:\Project\Flecs\bench\smoke_v14.exe" > smoke_v14_out.txt 2>&1
type smoke_v14_out.txt
echo === Tier-v14.1 source restored, smoke PASS ===