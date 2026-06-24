@echo off
setlocal
cd /d C:\Project\Flecs\bench
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >NUL 2>&1
if errorlevel 1 (
    echo [build_a3] vcvars64 failed
    exit /b 1
)

REM 1) Compile v18 + TIER-A3 patches to a fresh obj
cl /nologo /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v18.c /Fo:release\flecs_v18_a3.obj
if errorlevel 1 (
    echo [build_a3] cl compile failed with %errorlevel%
    exit /b 1
)

REM 2) Archive to lib
lib /nologo /OUT:release\v18_a3_flecs_patched.lib release\flecs_v18_a3.obj
if errorlevel 1 (
    echo [build_a3] lib failed with %errorlevel%
    exit /b 1
)

REM 3) Build benchmark
cl /nologo /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_a3_arena.c /Fe:release\test_a3_arena.exe release\v18_a3_flecs_patched.lib
if errorlevel 1 (
    echo [build_a3] benchmark compile failed with %errorlevel%
    exit /b 1
)

echo [build_a3] OK
exit /b 0