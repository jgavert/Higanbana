// entrypoint.cpp

#include "Platform/ProgramParams.hpp"
#include "Platform/EntryPoint.hpp" // client entrypoint


#ifdef WIN64
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	ProgramParams params(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
#else
int main(int argc, const char* argv[])
{
	ProgramParams params(argc, argv);
#endif
	EntryPoint ep(params);
	int returnValue = ep.main();
	return returnValue;
}
