@echo off
setlocal
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1
if errorlevel 1 (
    echo Failed to load vcvars64
    exit /b 1
)

echo === Build TIER-C1 patched library (flecs_patched_v18 + bitmap) ===
if not exist release mkdir release
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v18.c /Fo:release\flecs_v18_c1.obj 2>&1 | findstr /v "^Microsoft\|^Copyright\|^/out:"
if errorlevel 1 exit /b 1
lib /OUT:release\v18_c1_flecs_patched.lib release\flecs_v18_c1.obj
if errorlevel 1 exit /b 1

echo === Build TIER-C1 test ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_c1_bitmap.c /Fe:release\test_c1_bitmap.exe release\v18_c1_flecs_patched.lib 2>&1 | findstr /v "^Microsoft\|^Copyright\|^/out:"
if errorlevel 1 exit /b 1

echo === Run TIER-C1 test ===
".\release\test_c1_bitmap.exe"
echo === exit: %errorlevel% ===
endlocal
