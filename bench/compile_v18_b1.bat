@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ../include /c flecs_patched_v18.c /Fo:release\flecs_v18_b1.obj 2>release\compile_err.txt
echo RC=%ERRORLEVEL%
exit /b %ERRORLEVEL%
