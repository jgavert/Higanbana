//ProgramParams.hpp
#pragma once

#ifdef WIN64
#define NOMINMAX
#include <windows.h>
class ProgramParams
{
public:
  ProgramParams(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
    : m_hInstance(hInstance)
    , m_hPrevInstance(hPrevInstance)
    , m_lpCmdLine(lpCmdLine)
    , m_nCmdShow(nCmdShow)
  {}

  HINSTANCE m_hInstance;
  HINSTANCE m_hPrevInstance;
  LPSTR     m_lpCmdLine;
  int       m_nCmdShow;
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
