@echo off
cd /d "C:\Project\Flecs\distr"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Test: distr build with Tier-v14.1 patches (in flecs_patched.c)
REM Strategy: copy flecs_patched.c as distr/flecs.c, fix include, compile
if exist ..\bench\flecs_patched.c (
  copy /Y ..\bench\flecs_patched.c flecs.c >nul
  powershell -Command "(Get-Content flecs.c) -replace '#include \"flecs_patched.h\"', '#include \"flecs.h\"' | Set-Content flecs.c"
)

if exist flecs_test_v14.obj del flecs_test_v14.obj
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . flecs.c /Fo:flecs_test_v14.obj
if errorlevel 1 exit /b 1
lib /OUT:flecs_test_v14.lib flecs_test_v14.obj
if errorlevel 1 exit /b 1

REM Quick smoke: ecs_init/ecs_new/ecs_fini
if exist ..\bench\distr_smoke.c (
  cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . ..\bench\distr_smoke.c /Fe:..\bench\distr_smoke.exe flecs_test_v14.lib
  if errorlevel 1 exit /b 1
  "C:\Project\Flecs\bench\distr_smoke.exe"
  echo === Distr smoke PASS ===
)