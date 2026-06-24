@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Tier-v17 P0-2 syntax-only check (uses stub types, no Flecs deps)
if exist syntax_check_v17_p0_2.obj del syntax_check_v17_p0_2.obj
if exist syntax_check_v17_p0_2.exe del syntax_check_v17_p0_2.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I . syntax_check_v17_p0_2.c /Fe:syntax_check_v17_p0_2.exe
if errorlevel 1 exit /b 1

echo === syntax check built ===
syntax_check_v17_p0_2.exe
if errorlevel 1 (
    echo FAIL: helper returned wrong predicate
    exit /b 1
)
echo PASS: P0-2 helper compiles and returns expected predicate values