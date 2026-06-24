@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" 1>nul 2>&1
cd /d "C:\Project\Flecs\bench"
cl /O2 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /DFLECS_STATIC /I . /I ..\include /c flecs_patched_v19.c /Fo:flecs_v19_minimal_wc.obj
if errorlevel 1 exit /b 1
cl /O2 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /DFLECS_STATIC /I . /I ..\include /Fe:release\test_minimal_wildcard_v19.exe test_minimal_wildcard_v19.c flecs_v19_minimal_wc.obj
if errorlevel 1 exit /b 1
release\test_minimal_wildcard_v19.exe
exit /b %errorlevel%