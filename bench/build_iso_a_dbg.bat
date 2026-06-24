@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl /Od /W3 /D_CRT_SECURE_NO_WARNINGS /I . bench_iso_a.c flecs_patched.c /Fe:bench_iso_a_dbg.exe
echo === exit: %errorlevel% ===
