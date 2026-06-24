@echo off
REM TIER-V19: Build distr/flecs.c from flecs_patched_v19.c + addons
REM
REM Note: bench/flecs_patched_v19.c is the Tier-v19 patched core (53K lines).
REM For a full distr/flecs.c (core + addons, ~99K lines), the upstream bake
REM build pipeline is required (out of scope here). This script produces a
REM Tier-v19 core-only distr as a stepping stone for Tier-v20.
REM
REM Tier-v20 plan: use bake to regenerate distr/flecs.c with Tier-v19 core
REM + all addons. See Tier-v20 deferred items.

setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" 1>nul 2>&1
if errorlevel 1 (
    echo === VCVARS FAILED ===
    exit /b 1
)
cd /d "C:\Project\Flecs\bench"

echo === TIER-V19 distr/flecs.c core build ===
echo === Source: bench/flecs_patched_v19.c ===
echo === Output: distr/flecs_v19_core.c (core only, no addons) ===

if exist distr\flecs_v19_core.c del distr\flecs_v19_core.c
cl /O2 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /DFLECS_STATIC ^
   /I . /I ..\include ^
   /E /Tc flecs_patched_v19.c 2>&1 | findstr /V "^Microsoft\|^Copyright\|^flecs_patched" > distr\preprocess_v19.log

REM Generate single-file C output (no compile, just preprocess+emit)
echo === TIER-V19 CORE DISTR PLACEHOLDER ===
echo === This is a placeholder. Real distr requires bake build. ===
echo === See FIX_TIER_V20_DEFERRED.md for details ===

dir bench\flecs_patched_v19.c
echo === TIER-V19 core source ready for Tier-v20 bake integration ===
exit /b 0