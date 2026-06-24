@echo off
REM build_c2.bat — Tier-C2 tam surum build + benchmark
REM
REM Builds two flavors of flecs_patched_v18.c:
REM   1) v18_c2_off.lib  — baseline (FLECS_C2_PREFETCH undefined)
REM   2) v18_c2_on.lib   — with Tier-C2 tam surum prefetch (FLECS_C2_PREFETCH=1)
REM Then compiles bench_c2_prefetch.c twice (once against each lib) and runs both.

setlocal enabledelayedexpansion
cd /d "%~dp0"

if not exist release mkdir release

REM ---- baseline lib (no C2 prefetch) ----
echo [1/4] cl /c flecs_patched_v18.c (baseline)
cl /nologo /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c flecs_patched_v18.c /Fo:release\flecs_v18_c2_off.obj
if errorlevel 1 goto :err

echo [2/4] lib baseline
lib /nologo /OUT:release\v18_c2_off_flecs_patched.lib release\flecs_v18_c2_off.obj
if errorlevel 1 goto :err

REM ---- C2-on lib ----
echo [3/4] cl /c flecs_patched_v18.c (C2=ON)
cl /nologo /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /DFLECS_C2_PREFETCH=1 /I . /I ../include /c flecs_patched_v18.c /Fo:release\flecs_v18_c2_on.obj
if errorlevel 1 goto :err

echo [4/4] lib C2-on
lib /nologo /OUT:release\v18_c2_on_flecs_patched.lib release\flecs_v18_c2_on.obj
if errorlevel 1 goto :err

REM ---- bench exe: baseline ----
echo [B1/2] bench baseline
cl /nologo /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include bench_c2_prefetch.c /Fe:release\bench_c2_off.exe release\v18_c2_off_flecs_patched.lib
if errorlevel 1 goto :err

REM ---- bench exe: C2-on ----
echo [B2/2] bench C2-on
cl /nologo /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /DFLECS_C2_PREFETCH=1 /I . /I ../include bench_c2_prefetch.c /Fe:release\bench_c2_on.exe release\v18_c2_on_flecs_patched.lib
if errorlevel 1 goto :err

echo.
echo ========================================================
echo Run baseline (no C2 prefetch):
echo ========================================================
release\bench_c2_off.exe
if errorlevel 1 goto :err

echo.
echo ========================================================
echo Run C2-ON (Tier-C2 tam surum prefetch):
echo ========================================================
release\bench_c2_on.exe
if errorlevel 1 goto :err

echo.
echo ========================================================
echo COMPARISON (lower = faster)
echo ========================================================
echo Compare mean/best times above.  Same checksum => correctness OK.
exit /b 0

:err
echo BUILD FAILED
exit /b 1
