#pragma once
#include "ProgramParams.hpp"
#include "core/src/input/InputBuffer.hpp"
#include "core/src/math/math.hpp"
#include "core/src/datastructures/proxy.hpp"

#include <string>
#include <memory>

#ifdef FAZE_PLATFORM_WINDOWS
#include <Windowsx.h>
namespace faze
{
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
    WindowInternal(HWND hWnd, HINSTANCE hInstance, WNDCLASSEX wc, ProgramParams params, std::string className) :
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

class MouseState
{
public:
  int2 m_pos;
  int mouseWheel = 0;
  bool captured = true;
};

class Window
{
private:
  std::shared_ptr<WindowInternal> m_window;
  int m_width;
  int m_height;
  std::string m_name;
  bool needToResize = false;
  bool resizing = false;
  int m_resizeWidth = 0;
  int m_resizeHeight = 0;
  unsigned m_dpi = 96;
  bool m_minimized = false;
  bool m_windowVisible = false;
  bool m_borderlessFullscreen = false;
  RECT m_windowRect;

  BYTE m_oldKeyboardState[256];
  HHOOK m_keyboardHook;

  unsigned m_baseWindowFlags = WS_OVERLAPPEDWINDOW;

  int64_t m_frame = 0;
  faze::InputBuffer m_inputs;
  vector<wchar_t> m_characterInput;
  MouseState m_mouse;

  void keyDown(int key);
  void keyUp(int key);

  void resizeEvent(const char*  eventName);
  void setDpi(unsigned scale);
  void updateInputs();
public:
  Window(ProgramParams params, std::string windowName, int width, int height, int offsetX = 0, int offsetY = 0);
  bool open();
  void close();
  void cursorHidden(bool enabled);
  WindowInternal& getInternalWindow() { return *m_window; }
  bool simpleReadMessages(int64_t frame);

  void toggleBorderlessFullscreen();

  bool hasResized();
  void resizeHandled();

  void captureMouse(bool val);

  faze::InputBuffer& inputs()
  {
    return m_inputs;
  }

  MouseState& mouse()
  {
    return m_mouse;
  }

  vector<wchar_t> charInputs()
  {
    return m_characterInput;
  }

  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK LowLevelKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam);
};
}