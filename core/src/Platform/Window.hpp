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
  HINSTANCE hInstance;
  WNDCLASSEX wc;
  ProgramParams m_params;
  std::string m_className;

public:
  WindowInternal(HWND hWnd,HINSTANCE hInstance, WNDCLASSEX wc, ProgramParams params, std::string className) :
    hWnd(hWnd), hInstance(hInstance), wc(wc), m_params(params), m_className(className) {}
  ~WindowInternal()
  {
    DestroyWindow(hWnd);
  }
  HWND& getHWND() { return hWnd; }
  HINSTANCE& getHInstance() { return hInstance; }

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
  bool needToResize = false;
  bool resizing = false;
  int m_resizeWidth = 0;
  int m_resizeHeight = 0;
  unsigned m_dpi = 96;

  void resizeEvent(const char*  eventName);
  void setDpi(unsigned scale);
public:
	Window(ProgramParams params, std::string windowName, int width, int height);
  bool open();
  void close();
  void cursorHidden(bool enabled);
  WindowInternal& getInternalWindow() { return *m_window; }
  bool simpleReadMessages();

  bool hasResized();
  void resizeHandled();

  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

