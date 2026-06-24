# PGO Build Pipeline (D2 Tier)

PGO (Profile-Guided Optimization) gives +%10-20 hot-path bonus on top of /O2.

## Quick start

```cmd
cd bench
build_pgo_full.bat
```

This runs:
1. build_pgo_instr.bat — instrumented build with /GL
2. build_pgo_run.bat — runs bench 5x to generate .pgc profile data
3. build_pgo_optimized.bat — re-builds using /LTCG:PGORT with profile

Output: `bench/v18_pgo_flecs_patched.lib` (PGO-optimized)

## Manual steps

If automated fails, run each bat individually.

## CMake preset

Use `cmake --preset flecs-perf-pgo && cmake --build build` for in-source PGO.

## Notes

- Requires MSVC 19.x with /LTCG support
- /arch:AVX2 requires CPU with AVX2 (Intel Haswell+ / AMD Excavator+)
- /favor:INTEL optimizes for Intel microarchitecture (slight loss on AMD)