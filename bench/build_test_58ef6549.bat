@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" 1>nul 2>&1
if errorlevel 1 (
    echo === VCVARS FAILED ===
    exit /b 1
)
cd /d "C:\Project\Flecs\bench"
echo === Compiling test_query_wildcard_dontfragment.c ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /DFLECS_STATIC ^
   /I . /I ..\include /c flecs_patched_v19.c /Fo:flecs_v19_test.obj
if errorlevel 1 exit /b 1
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /DFLECS_STATIC ^
   /I . /I ..\include ^
   /Fe:release\test_query_wildcard_dontfragment.exe ^
   test_query_wildcard_dontfragment.c ^
   flecs_v19_test.obj
if errorlevel 1 exit /b 1
echo === Running test ===
release\test_query_wildcard_dontfragment.exe
exit /b %errorlevel%