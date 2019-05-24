#pragma once
#include "core/platform/definitions.hpp"
#include "core/platform/ProgramParams.hpp"
#include "core/input/InputBuffer.hpp"
#include "core/math/math.hpp"
#include "core/datastructures/proxy.hpp"

#include <string>
#include <memory>

#include <atomic>

namespace faze
{
#ifdef FAZE_PLATFORM_WINDOWS
#include <Windows.h>
#include <Windowsx.h>
#include <winuser.h>

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

#ifdef FAZE_PLATFORM_WINDOWS
  class Window
  {
  private:
    std::shared_ptr<WindowInternal> m_window;
    int m_width;
    int m_height;
    std::string m_name;
    std::atomic<bool> needToResize = false;
    std::atomic<bool> resetMousePosToMiddle = false;
    bool resizing = false;
    int m_resizeWidth = 0;
    int m_resizeHeight = 0;
    unsigned m_dpi = 96;
    std::atomic<bool>  m_minimized = false;
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

    // touch
    struct TouchEvent
    {
      int uid = -1;
      int x = -1;
      int y = -1;
      int index = -1;
      bool touched = false;
      TouchEvent(int uid, int x, int y, int index, bool touched)
        : uid(uid)
        , x(x)
        , y(y)
        , index(index)
        , touched(touched)
      {

      }
    };

    vector<TouchEvent> touchEvents;

    void prepareTouchInputs()
    {
      for (auto&& it : touchEvents)
      {
        it.touched = false;
      }
    }

    void cleanTouchInputs()
    {
      touchEvents.erase(std::remove_if(touchEvents.begin(),
        touchEvents.end(),
        [](TouchEvent x) {return !x.touched; }),
        touchEvents.end());
    }

    void updateTouchInput(int uid, int x, int y)
    {
      bool found = false;
      for (auto&& it : touchEvents)
      {
        if (it.uid == uid)
        {
          it.x = x;
          it.y = y;
          it.touched = true;
          found = true;
          break;
        }
      }

      if (!found)
      {
        auto id = findUnusedIndex();
        touchEvents.push_back(TouchEvent(uid, x, y, id, true));
      }
    }

    int findUnusedIndex()
    {
      int index = 0;
      bool used = true;
      while (used)
      {
        used = false;
        for (auto&& it : touchEvents)
        {
          if (it.index == index)
          {
            index++;
            used = true;
            break;
          }
        }
      }
      return index;
    }

    UINT cInputs;
    TOUCHINPUT pInputs;
    POINT ptInput;

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
#else
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

    int64_t m_frame = 0;
    faze::InputBuffer m_inputs;
    faze::vector<char> m_characterInput;
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

    faze::vector<char> charInputs()
    {
      return m_characterInput;
    }
  };
#endif
}