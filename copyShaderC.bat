set cachefolder="C:/toki/code/shaderc"
IF EXIST %cachefolder% (
	RMDIR /S /Q externals/shaderc
	MKDIR externals/shaderc
	MKDIR externals/shaderc/lib
	MKDIR externals/shaderc/include 
	COPY %cachefolder%/build/libshaderc/RelWithDebInfo/shaderc_combined.lib externals/shaderc/lib/shaderc_combined_r.lib
	COPY %cachefolder%/build/libshaderc/Debug/shaderc_combined.lib externals/shaderc/lib/shaderc_combined_d.lib
	ROBOCOPY %cachefolder%/libshaderc/include externals/shaderc/include /E
)