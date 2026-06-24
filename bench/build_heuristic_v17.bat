@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >/dev/null 2>&1
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_a13_only.c /Fe:test_a13_only_v17p22.exe flecs_v17_p22.obj
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include test_auto_dont_fragment_lazy.c /Fe:test_auto_dont_fragment_lazy_v17p22.exe flecs_v17_p22.obj
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include bench_ecs_data_component.c /Fe:bench_ecs_data_component_v17p22.exe flecs_v17_p22.obj
