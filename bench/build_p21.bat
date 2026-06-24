@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
echo === P2-1 entity-index flat array build ===
if exist release\v18_p21_flecs_patched.lib del release\v18_p21_flecs_patched.lib
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include /c flecs_patched_v18.c /Fo:flecs_v18_p21.obj 2>&1
if errorlevel 1 (
    echo === COMPILE FAILED ===
    exit /b 1
)
lib /OUT:release\v18_p21_flecs_patched.lib flecs_v18_p21.obj
if errorlevel 1 (
    echo === LIB FAILED ===
    exit /b 1
)
echo === Building test_entity_index_flat.exe ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_entity_index_flat.c /Fe:release\test_entity_index_flat.exe flecs_v18_p21.obj
if errorlevel 1 (
    echo === TEST BUILD FAILED ===
    exit /b 1
)
echo === Running test ===
release\test_entity_index_flat.exe
if errorlevel 1 (
    echo === TEST FAILED ===
    exit /b 1
)
echo === P2-1 PASS ===
exit /b 0