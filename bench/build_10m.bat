@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD ^
   /I . /I ../include test_10m_stress.c /Fe:test_10m_stress.exe ^
   release\flecs_patched.lib
echo === BUILD OK ===