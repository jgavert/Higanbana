@echo off

IF EXIST "%BAZEL_VC%" GOTO INFO
IF EXIST C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC GOTO COMMUNITY_VS
IF EXIST C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC GOTO PROFESSIONAL_VS
GOTO ERROR

:COMMUNITY_VS
set BAZEL_VC=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC
echo found community edition, using vs2019 community
GOTO END

:PROFESSIONAL_VS
set BAZEL_VC=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC
echo found Professional edition, using vs2019 Professional
GOTO END

:ERROR
echo No compliant visual studio found :(( plz get vs2019
pause
exit(1)

:INFO
echo using BAZEL_VC=%BAZEL_VC%

:END
