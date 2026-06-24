@echo off
call build_pgo_instr.bat || exit /b 1
call build_pgo_run.bat || exit /b 1
call build_pgo_optimized.bat || exit /b 1
echo === PGO FULL PIPELINE COMPLETE ===
exit /b 0