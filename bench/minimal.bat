@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1

echo === Build minimal baseline ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I ../distr minimal_test.c ../distr/flecs_no_addons.c /Fe:minimal_baseline.exe 2>&1 | findstr /v "^Microsoft\|^Copyright\|^/out:\|Telif"
if errorlevel 1 exit /b 1

echo === Build minimal patched ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I . minimal_test_patched.c flecs_patched.c /Fe:minimal_patched.exe 2>&1 | findstr /v "^Microsoft\|^Copyright\|^/out:\|Telif"
if errorlevel 1 exit /b 1

echo === Run baseline ===
".\minimal_baseline.exe"
echo === exit: %errorlevel% ===
echo.
echo === Run patched ===
".\minimal_patched.exe"
echo === exit: %errorlevel% ===
