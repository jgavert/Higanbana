#pragma once
#include "ProgramParams.hpp"
#include <string>

#ifdef WIN64

class Window
{
private:
	HWND hWnd;
	WNDCLASSEX wc;
	ProgramParams m_params;
  std::string m_className;
public:
	Window(ProgramParams params, std::string className);
  bool open(std::string windowname, int width, int height);
  void cursorHidden(bool enabled);
  HWND& getNative() { return hWnd; }
};

#endif

