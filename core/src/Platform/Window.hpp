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
  HWND& getNative() { return hWnd; }
};
#else
class WindowInternal
{
private:
  friend class Window;
  int m_ptr;
  ProgramParams m_params;
  std::string m_className;

public:
  WindowInternal(ProgramParams params, std::string className)
    : m_ptr(0)
    , m_params(params)
    , m_className(className)
  {
  }

  int& getNative() { return m_ptr; }

};
#endif

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
  WindowInternal& getInternalWindow() { return *m_window; }
  bool simpleReadMessages();
  int width() { return m_width; }
  int height() { return m_height; }
};

