@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >/dev/null 2>&1

REM Heuristic + DC tests against v17_p22
for %%T in (test_a13_only test_auto_dont_fragment_lazy bench_ecs_data_component) do (
    if exist %%T_v17_p22.exe del %%T_v17_p22.exe
    cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include %%T.c /Fe:%%T_v17_p22.exe flecs_v17_p22.obj 2>/dev/null
)

echo --- test_a13_only ---
test_a13_only_v17_p22.exe 2>&1 | tail -3
echo --- test_auto_dont_fragment_lazy ---
test_auto_dont_fragment_lazy_v17_p22.exe 2>&1 | tail -3
echo --- bench_ecs_data_component (DC test) ---
bench_ecs_data_component_v17_p22.exe 2>&1 | tail -3
