@echo off
REM Scalar-only baseline (no FLECS_C3_SIMD).
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl /nologo /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD ^
   /I . /I ..\include ^
   /c flecs_patched_v18.c /Fo:release\flecs_v18_scalar.obj
if errorlevel 1 exit /b 1
if exist release\v18_scalar_flecs_patched.lib del release\v18_scalar_flecs_patched.lib
lib /NOLOGO /OUT:release\v18_scalar_flecs_patched.lib release\flecs_v18_scalar.obj
cl /nologo /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD ^
   /I . /I ..\include ^
   test_c3_simd.c release\v18_scalar_flecs_patched.lib /Fe:release\test_c3_simd_scalar.exe
release\test_c3_simd_scalar.exe