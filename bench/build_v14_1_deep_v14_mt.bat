@echo off
setlocal
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (echo VCVARS FAILED & exit /b 1)
del /Q test_v14_1_deep_v14_mt.exe test_v14_1_deep_v14_mt.obj 2>nul
cl /O2 /EHsc /std:c++20 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD ^
   /I . /I ..\include test_v14_1_deep_v14_mt.cpp ^
   /Fe:test_v14_1_deep_v14_mt.exe ^
   release\flecs_patched.lib
if errorlevel 1 (echo === BUILD FAILED === & exit /b 1)
echo === BUILD OK ===
endlocal