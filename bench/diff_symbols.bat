@echo off
cd /d "C:\Project\Flecs\bench"
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
echo === v17 symbols matching 'flecs_query_iter ' ===
dumpbin /SYMBOLS flecs_v17.obj | findstr /C:"flecs_query_iter "
echo === v17_p12 symbols matching 'flecs_query_iter ' ===
dumpbin /SYMBOLS flecs_v17_p12.obj | findstr /C:"flecs_query_iter "
echo === diff in symbol counts ===
dumpbin /SYMBOLS flecs_v17.obj > tmp_v17.txt
dumpbin /SYMBOLS flecs_v17_p12.obj > tmp_v17p12.txt
fc tmp_v17.txt tmp_v17p12.txt | head -20