@echo off
setlocal
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo === Could not load vcvars64.bat — try without vcvars ===
)

set SRC=flecs_patched_v18.c
set LIB_OUT=release\v18_e2e3_flecs_patched.lib
set EXE=release\test_e2_e3.exe

echo === Compiling %SRC% with E2 + E3 patches ===
if exist release\flecs_v18_e2e3.obj del release\flecs_v18_e2e3.obj
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c %SRC% /Fo:release\flecs_v18_e2e3.obj 2>&1
if errorlevel 1 (
    echo === COMPILE FAILED ===
    exit /b 1
)
if exist %LIB_OUT% del %LIB_OUT%
lib /OUT:%LIB_OUT% release\flecs_v18_e2e3.obj
if errorlevel 1 (
    echo === LIB FAILED ===
    exit /b 1
)
echo === lib OK: %LIB_OUT% ===

echo === Building test_e2_e3.exe ===
if exist %EXE% del %EXE%
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include test_e2_e3.c /Fe:%EXE% %LIB_OUT%
if errorlevel 1 (
    echo === EXE BUILD FAILED ===
    exit /b 1
)
echo === EXE OK: %EXE% ===
echo.
echo === Running test_e2_e3.exe ===
%EXE%
set RC=%ERRORLEVEL%
echo === test_e2_e3.exe returned %RC% ===
exit /b %RC%
