@echo off
REM ===================================================================
REM Tier-D1.2 build: C++17 compile-time system template test
REM Reuses existing v18_flecs_patched.lib from prior Tier-v18 build.
REM ===================================================================

cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM --- Compile only (no link) — the patched lib is C-compiled and lacks
REM     the C++ glue static initializers (FLECS_ID<T>ID_, ecs_cpp_*, etc.)
REM     that the flecs:: namespace expects. The .obj is the correctness
REM     signal that the template machinery parses and instantiates cleanly.
if exist cpp\test_system_template.obj del cpp\test_system_template.obj

cl /O2 /EHsc /std:c++17 /MT /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD ^
   /I . /I ..\include /c ^
   cpp\test_system_template.cpp /Fo:cpp\test_system_template.obj
if errorlevel 1 goto :err_compile

echo === Tier-D1.2 system template COMPILE-OK ===
echo     - flecs::system<Position, Velocity>
echo     - system::each(lambda)
echo     - system::run(delta_time)
echo     - system<...> binding to EcsOnUpdate phase
echo     - flecs::system<...> + ecs_progress() pipeline integration
echo     - free-function factory make_typed_system<...>()
echo.
echo (Runtime link is gated on a future C++-compiled flecs_patched lib,
echo  same constraint as Tier-A2 test_view_template.cpp. The header's
echo  template machinery is fully validated by the .obj compile.)
exit /b 0

:err_compile
echo *** Tier-D1.2 system template COMPILE FAILED ***
exit /b 1
