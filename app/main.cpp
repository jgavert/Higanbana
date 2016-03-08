// entrypoint.cpp
#ifdef WIN64
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "core/src/Platform/ProgramParams.hpp"
#include "core/src/Platform/EntryPoint.hpp" // client entrypoint


#ifdef WIN64
int WINAPI WinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  // Get current flag
  //int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

  // Turn on leak-checking bit.
  //tmpFlag |= _CRTDBG_LEAK_CHECK_DF;

  // Turn off CRT block checking bit.
  //tmpFlag &= ~_CRTDBG_CHECK_CRT_DF;

  // Set flag to the new value.
  //_CrtSetDbgFlag(tmpFlag);
  int returnValue = 0;
  {
    ProgramParams params(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
#else
int main(int argc, char** argv)
{
  int returnValue = 0;
  {
    ProgramParams params(argc, argv);
#endif
    EntryPoint ep(params);

    returnValue = ep.main();
  }

#ifdef WIN64
  //_CrtDumpMemoryLeaks(); // finds my hack message class thingies from globalspace.
#endif
  return returnValue;
}
