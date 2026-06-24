@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if exist release\v17_flecs_patched.lib del release\v17_flecs_patched.lib
lib /OUT:release\v17_flecs_patched.lib flecs_v17.obj
if errorlevel 1 exit /b 1
echo === v17 lib OK ===
dir release\v17_flecs_patched.lib