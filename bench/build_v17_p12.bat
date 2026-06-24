@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if exist flecs_v17_p12.obj del flecs_v17_p12.obj
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v17_p12.c /Fo:flecs_v17_p12.obj
if errorlevel 1 exit /b 1
if exist release\v17_p12_flecs_patched.lib del release\v17_p12_flecs_patched.lib
lib /OUT:release\v17_p12_flecs_patched.lib flecs_v17_p12.obj
if errorlevel 1 exit /b 1
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include bench_trivial_ctx.c /Fe:release\bench_trivial_v17p12.exe flecs_v17_p12.obj
if errorlevel 1 exit /b 1
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include diag_v18.c /Fe:diag_v17_p12.exe flecs_v17_p12.obj
if errorlevel 1 exit /b 1
echo === v17_p12 lib + binaries built ===
dir release\v17_p12_flecs_patched.lib release\bench_trivial_v17p12.exe diag_v17_p12.exe