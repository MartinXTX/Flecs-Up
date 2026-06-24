@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if exist bench_override_v17.exe del bench_override_v17.exe
if exist bench_override_v18.exe del bench_override_v18.exe
echo === Building bench_override v17 ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include bench_override.c /Fe:bench_override_v17.exe release\v17_flecs_patched.lib 1>nul
echo v17 build RC=%ERRORLEVEL%
if exist bench_override_v17.exe (
    echo === v17 EXE built ===
) else (
    echo === v17 EXE missing ===
)
echo === Building bench_override v18 ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include bench_override.c /Fe:bench_override_v18.exe release\v18_flecs_patched.lib 1>nul
echo v18 build RC=%ERRORLEVEL%
if exist bench_override_v18.exe (
    echo === v18 EXE built ===
) else (
    echo === v18 EXE missing ===
)

