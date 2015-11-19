#pragma once
#include "ProgramParams.hpp"
#include <string>
#include <memory>

#ifdef WIN64

class WindowInternal
{
private:
  friend class Window;
  HWND hWnd;
  WNDCLASSEX wc;
  ProgramParams m_params;
  std::string m_className;

public:
  WindowInternal(HWND hWnd, WNDCLASSEX wc, ProgramParams params, std::string className) :
    hWnd(hWnd), wc(wc), m_params(params), m_className(className) {}
  ~WindowInternal()
  {
    DestroyWindow(hWnd);
  }
};

class Window
{
private:
  std::shared_ptr<WindowInternal> m_window;
  int m_width;
  int m_height;
public:
	Window(ProgramParams params, std::string windowName, int width, int height);
  bool open();
  void close();
  void cursorHidden(bool enabled);
  HWND& getNative() { return m_window->hWnd; }
  bool simpleReadMessages();
};

#endif

