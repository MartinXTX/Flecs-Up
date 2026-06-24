@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" 1>nul 2>&1
if errorlevel 1 (
    echo === VCVARS FAILED ===
    exit /b 1
)
cd /d "C:\Project\Flecs\bench"
echo === Compiling test_e2_e3.c ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /DFLECS_STATIC /I . /I ..\include /c test_e2_e3.c /Fo:release\test_e2_e3.obj
if errorlevel 1 (
    echo === COMPILE FAILED ===
    exit /b 1
)
echo === Linking test_e2_e3.exe ===
link /OUT:release\test_e2_e3.exe release\test_e2_e3.obj release\v19_flecs_patched.lib
if errorlevel 1 (
    echo === LINK FAILED ===
    exit /b 1
)
echo === Running test ===
release\test_e2_e3.exe
exit /b %errorlevel%