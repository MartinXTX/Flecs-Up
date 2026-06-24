@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Rebuild Tier-v16 production lib (with auto-mark fix)
if exist flecs_v16.obj del flecs_v16.obj
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v16.c /Fo:flecs_v16.obj
if errorlevel 1 exit /b 1
lib /OUT:release\v16_flecs_patched.lib flecs_v16.obj
if errorlevel 1 exit /b 1

REM Rebuild benchmark EXE
if exist release\v16_bench_flecs.exe del release\v16_bench_flecs.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include bench_flecs.c /Fe:release\v16_bench_flecs.exe flecs_v16.obj
if errorlevel 1 exit /b 1

REM Promote to release/
copy /Y release\v16_flecs_patched.lib release\flecs_patched.lib >nul
copy /Y release\v16_bench_flecs.exe release\bench_flecs.exe >nul

echo === Tier-v16 production lib + EXE rebuilt with auto-mark fix ===
dir release\flecs_patched.lib release\bench_flecs.exe