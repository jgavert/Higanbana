CALL FBuild.exe -dist App-x64-Release
rmdir /s /q package
mkdir package
ROBOCOPY bin/x64-Release/App/ package/ App.exe
ROBOCOPY appspace/ package/ /E