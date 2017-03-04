CALL python generateConfig.py
CALL FBuild.exe -clean proj
set cachefolder="C:/temp/fazecache"
IF EXIST %cachefolder% (
	cd /d %cachefolder%
	for /F "delims=" %%i in ('dir /b') do (rmdir "%%i" /s/q || del "%%i" /s/q)
)