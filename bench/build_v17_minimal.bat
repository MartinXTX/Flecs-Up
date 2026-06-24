@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include bench_minimal_v17.c /Fe:bench_minimal_v17.exe release\v17_flecs_patched.lib 1>nul
echo Build: %ERRORLEVEL%
.\bench_minimal_v17.exe
echo --- EXIT: %ERRORLEVEL% ---