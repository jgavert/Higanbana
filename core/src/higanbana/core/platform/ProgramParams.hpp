#pragma once
#include "higanbana/core/platform/definitions.hpp" 
#ifdef HIGANBANA_PLATFORM_WINDOWS 
#include <windows.h>
#include <ShellScalingAPI.h>
#include <iostream>

class ProgramParams
{
public:
  ProgramParams(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow, bool extraConsole = true)
    : m_hInstance(hInstance)
    , m_hPrevInstance(hPrevInstance)
    , m_lpCmdLine(lpCmdLine)
    , m_nCmdShow(nCmdShow)
    , m_argc(__argc)
    , m_argv(__argv)
  {
	  // super cool!
    if (false && extraConsole) {
      AllocConsole();
      freopen("CONIN$", "r", stdin);
      freopen("CONOUT$", "w", stdout);
      freopen("CONOUT$", "w", stderr);
      std::wcout.clear();
      std::cout.clear();
      std::wcerr.clear();
      std::cerr.clear();
      std::wcin.clear();
      std::cin.clear();
    }
  }

  HINSTANCE m_hInstance;
  HINSTANCE m_hPrevInstance;
  LPSTR    m_lpCmdLine;
  int       m_nCmdShow;

  int         m_argc;
  char**      m_argv;
};

#else

struct ProgramParams
{
  int         m_argc;
  char**      m_argv;

  ProgramParams(int argc, char** argv)
    : m_argc(argc)
    , m_argv(argv)
  {}
};
#endif
