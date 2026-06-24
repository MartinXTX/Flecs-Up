@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
echo === Building test_correctness v18 ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include test_correctness.c /Fe:test_correctness_v18.exe release\v18_flecs_patched.lib 1>nul
echo v18 build RC=%ERRORLEVEL%
if exist test_correctness_v18.exe (
    echo === v18 EXE built, running ===
    ".\test_correctness_v18.exe" > test_correctness_v18_out.txt 2>&1
    echo === Last 20 lines of output ===
    powershell -command "Get-Content test_correctness_v18_out.txt | Select-Object -Last 20"
) else (
    echo === v18 EXE missing ===
)
