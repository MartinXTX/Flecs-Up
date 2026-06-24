@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
echo === Building 5-suite tests against v17 lib ===
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include test_correctness.c /Fe:test_correctness_tier_a1.exe release\v17_flecs_patched.lib 2>&1
echo correctness: %ERRORLEVEL%
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include test_stress.c /Fe:test_stress_tier_a1.exe release\v17_flecs_patched.lib 2>&1
echo stress: %ERRORLEVEL%
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include test_observer.c /Fe:test_observer_tier_a1.exe release\v17_flecs_patched.lib 2>&1
echo observer: %ERRORLEVEL%
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include test_edge.c /Fe:test_edge_tier_a1.exe release\v17_flecs_patched.lib 2>&1
echo edge: %ERRORLEVEL%
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include test_stability.c /Fe:test_stability_tier_a1.exe release\v17_flecs_patched.lib 2>&1
echo stability: %ERRORLEVEL%
