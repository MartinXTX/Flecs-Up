@echo off
REM build_b2.bat - build Tier-B2 patched flecs + test
REM
REM This script builds the TIER-B2 tiny-archetype single-chunk variant of
REM flecs_patched_v18.c (compile + lib + test_b2_tiny.exe) using MSVC.
REM
REM Pre-req: cl and lib in PATH (vcvars64.bat has been sourced).
REM Pre-req: TIER-B2 patches already applied to:
REM   - bench/flecs_patched_v18.c     (single-header distro)
REM   - bench/flecs_patched.h         (EcsTableIsTiny flag)
REM   - src/storage/table.c           (upstream canonical)
REM   - src/storage/table.h           (ecs_table__t fields)

setlocal

call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >NUL 2>&1

REM Echo env to debug
echo INCLUDE=%INCLUDE:;= ;%
echo LIB=%LIB%
echo LIBPATH=%LIBPATH%

set FLECS=C:\Project\Flecs
set SRC=%FLECS%\bench\flecs_patched_v18.c
set OBJ=%FLECS%\bench\release\flecs_v18_b2.obj
set LIB=%FLECS%\bench\release\v18_b2_flecs_patched.lib
set TEST_SRC=%FLECS%\bench\test_b2_tiny.c
set TEST_EXE=%FLECS%\bench\release\test_b2_tiny.exe

if not exist "%FLECS%\bench\release" mkdir "%FLECS%\bench\release"

echo [1/4] compile %SRC%
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD ^
   /I "%FLECS%" /I "%FLECS%\include" ^
   /c "%SRC%" /Fo:"%OBJ%"
if errorlevel 1 (
    echo compile failed
    exit /b 1
)

echo [2/4] archive %LIB%
lib /OUT:"%LIB%" "%OBJ%"
if errorlevel 1 (
    echo lib failed
    exit /b 1
)

echo [3/4] compile + link %TEST_SRC%
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /MD ^
   /I "%FLECS%" /I "%FLECS%\include" ^
   "%TEST_SRC%" /Fe:"%TEST_EXE%" "%LIB%" ^
   /link /LIBPATH:"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\14.44.35207\lib\x64" /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64" /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64"
if errorlevel 1 (
    echo test compile/link failed
    exit /b 1
)

echo [4/4] run %TEST_EXE%
"%TEST_EXE%"
exit /b %ERRORLEVEL%