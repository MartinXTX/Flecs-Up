@echo off
REM ===================================================================
REM BULGU-41 build script
REM Compiles the patched Tier-v18 source with BULGU-41 fix, links into
REM a static library, then builds the regression test executable.
REM
REM Uses Visual Studio 18 (Enterprise) at:
REM   C:\Program Files\Microsoft Visual Studio\18\Enterprise
REM (matches the existing bench/build.bat and the previously-built v18 lib).
REM
REM Output:
REM   bench/release/flecs_v18_bulgu41.obj         - patched object file
REM   bench/release/v18_bulgu41_flecs_patched.lib - patched static library
REM   bench/release/test_bulgu_41_pair.exe       - regression test EXE
REM
REM Usage:
REM   bench\build_bulgu_41.bat
REM ===================================================================

setlocal enabledelayedexpansion

set MSVC_ROOT=C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Tools\MSVC\14.50.35717
set SDK_ROOT=C:\Program Files (x86)\Windows Kits\10
set SDK_VER=10.0.26100.0

set PATH=%MSVC_ROOT%\bin\Hostx64\x64;%SDK_ROOT%\bin\%SDK_VER%\x64;%PATH%
set INCLUDE=%MSVC_ROOT%\include;%SDK_ROOT%\Include\%SDK_VER%\ucrt;%SDK_ROOT%\Include\%SDK_VER%\um;%SDK_ROOT%\Include\%SDK_VER%\shared;%SDK_ROOT%\Include\%SDK_VER%\winrt
set LIB=%MSVC_ROOT%\lib\x64;%SDK_ROOT%\Lib\%SDK_VER%\ucrt\x64;%SDK_ROOT%\Lib\%SDK_VER%\um\x64

set FLECS_DIR=C:\Project\Flecs
set BENCH_DIR=C:\Project\Flecs\bench
set RELEASE_DIR=C:\Project\Flecs\bench\release

if not exist "%RELEASE_DIR%" mkdir "%RELEASE_DIR%"

echo === BULGU-41: compiling flecs_patched_v18.c ===
pushd "%FLECS_DIR%"
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /nologo ^
   /I "%FLECS_DIR%" /I "%FLECS_DIR%\include" /I "%BENCH_DIR%" ^
   /c "%BENCH_DIR%\flecs_patched_v18.c" ^
   /Fo:"%RELEASE_DIR%\flecs_v18_bulgu41.obj"
if errorlevel 1 ( popd & echo CL_FAIL & exit /b 1 )
popd

echo === BULGU-41: building static library ===
lib /nologo /OUT:"%RELEASE_DIR%\v18_bulgu41_flecs_patched.lib" ^
       "%RELEASE_DIR%\flecs_v18_bulgu41.obj"
if errorlevel 1 ( echo LIB_FAIL & exit /b 1 )

echo === BULGU-41: building regression test ===
pushd "%FLECS_DIR%"
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /nologo ^
   /I "%FLECS_DIR%" /I "%FLECS_DIR%\include" /I "%BENCH_DIR%" ^
   "%BENCH_DIR%\test_bulgu_41_pair.c" ^
   /Fe:"%RELEASE_DIR%\test_bulgu_41_pair.exe" ^
   "%RELEASE_DIR%\v18_bulgu41_flecs_patched.lib"
if errorlevel 1 ( popd & echo TEST_LINK_FAIL & exit /b 1 )
popd

echo === BULGU-41: running regression test ===
"%RELEASE_DIR%\test_bulgu_41_pair.exe"
if errorlevel 1 ( echo TEST_FAIL & exit /b 1 )

echo === BULGU-41: ALL PASS ===
exit /b 0