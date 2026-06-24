@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >/dev/null 2>&1

REM Build the 5-suite test EXEs against v17_p22 lib
for %%T in (test_correctness test_edge test_observer test_stability test_stress) do (
    echo Building %%T_v17_p22.exe ...
    if exist %%T_v17_p22.exe del %%T_v17_p22.exe
    cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include %%T.c /Fe:%%T_v17_p22.exe flecs_v17_p22.obj
    if errorlevel 1 echo %%T BUILD FAILED
)

REM Run all 5 suites
echo === Running 5-suite regression against v17_p22 ===
set PASS=0
set FAIL=0
for %%T in (test_correctness test_edge test_observer test_stability test_stress) do (
    if exist %%T_v17_p22.exe (
        echo --- %%T_v17_p22.exe ---
        %%T_v17_p22.exe 2>&1 | findstr /C:"PASS" /C:"FAIL" /C:"ERROR" | findstr /V "PASS" >/dev/null
        if errorlevel 1 (
            echo %%T: PASS
            set /a PASS+=1
        ) else (
            echo %%T: FAIL
            set /a FAIL+=1
        )
    )
)
echo === Results: %PASS% pass, %FAIL% fail ===
