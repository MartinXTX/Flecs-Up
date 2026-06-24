@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >/dev/null 2>&1
REM Build bench_arena against v17_p22
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include bench_arena.c /Fe:bench_arena_v17p22.exe flecs_v17_p22.obj
REM Build bench_arena against v16
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include bench_arena.c /Fe:bench_arena_v16.exe flecs_v16.obj
