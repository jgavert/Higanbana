set cachefolder="C:\toki\code\shaderc"
IF EXIST %cachefolder% (
	RMDIR externals\shaderc /S /Q 
	MKDIR externals\shaderc
	MKDIR externals\shaderc\lib
	MKDIR externals\shaderc\include 
	COPY %cachefolder%\build\libshaderc\RelWithDebInfo\shaderc_combined.lib externals\shaderc\lib\shaderc_combined_r.lib
	COPY %cachefolder%\build\libshaderc\RelWithDebInfo\shaderc.lib externals\shaderc\lib\shaderc_r.lib
	COPY %cachefolder%\build\libshaderc\Debug\shaderc_combined.lib externals\shaderc\lib\shaderc_combined_d.lib
	COPY %cachefolder%\build\libshaderc\Debug\shadercd.lib externals\shaderc\lib\shaderc_d.lib
	ROBOCOPY %cachefolder%\libshaderc\include externals\shaderc\include /E
)