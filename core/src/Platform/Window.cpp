#include "Window.hpp"

#include "core/src/global_debug.hpp"

#if defined(PLATFORM_WINDOWS)
LRESULT CALLBACK Window::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  Window* me = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
  switch (message)
  {
  case WM_DESTROY:
    {
      PostQuitMessage(0);
      return 0;
    }
  case WM_ENTERSIZEMOVE:
  {
    if (me)
    {
      me->resizing = true;
    }
    break;
  }
  case WM_EXITSIZEMOVE:
  {
    if (me)
    {
      me->resizeEvent("Resized");
      me->resizing = false;
    }
    break;
  }
  case WM_KEYDOWN:
  {
    if (me)
    {
      me->keyDown(static_cast<int>(wParam));
    }
    break;
  }
  case WM_KEYUP:
  {
    if (me)
    {
      me->keyUp(static_cast<int>(wParam));
    }
    break;
  }
  case WM_SIZE:
  {
    if (me)
    {
      auto evnt = static_cast<int>(wParam);
      if (SIZE_MINIMIZED == evnt)
      {
        me->m_minimized = true;
        me->resizeEvent("Minimized");
      }

      if (!me->m_minimized)
      {
        me->m_resizeWidth = static_cast<int>(LOWORD(lParam));
        me->m_resizeHeight = static_cast<int>(HIWORD(lParam));
      }

      if (SIZE_MAXIMIZED == evnt)
      {
        me->m_minimized = false;
        me->resizeEvent("Maximized");
      }
      else if (SIZE_RESTORED == evnt && !me->resizing)
      {
        me->m_minimized = false;
        me->resizeEvent("Restored");
      }
    }
  }
  case WM_DPICHANGED:
  {
    if (me)
    {
      auto dpi = LOWORD(wParam);
      if (dpi == 0)
        break;
      me->setDpi(dpi);
      //RECT* lprcNewScale = reinterpret_cast<RECT*>(lParam);
    }
  }
  default:
    break;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

void Window::resizeEvent(const char* eventName)
{
  if ((m_width != m_resizeWidth || m_height != m_resizeHeight) && !m_minimized)
  {
    F_SLOG("Window", "%s %dx%d\n",eventName, m_resizeWidth, m_resizeHeight);
    needToResize = true;
  }
}

void Window::setDpi(unsigned scale)
{
  m_dpi = scale;
}

bool Window::hasResized()
{
  return needToResize && !m_minimized;
}

void Window::resizeHandled()
{
  needToResize = false;
  m_width = m_resizeWidth;
  m_height = m_resizeHeight;
}

void Window::keyDown(int key)
{
  m_inputs.insert(key, 1, m_frame);
}
void Window::keyUp(int key)
{
  m_inputs.insert(key, 0, m_frame);
}

#endif

// Initializes the window and shows it
Window::Window(ProgramParams params, std::string windowname, int width, int height)
  : m_width(width)
  , m_height(height)
{
#if defined(PLATFORM_WINDOWS)
  WNDCLASSEX wc;
  HWND hWnd;
  ZeroMemory(&hWnd, sizeof(HWND));
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

  std::string classname = (windowname + "class");

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = params.m_hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
  wc.lpszClassName = classname.c_str();
  RegisterClassEx(&wc);

  RECT wr = { 0, 0, width, height };
  AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

  auto result = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
  if (result != S_OK)
  {
    F_SLOG("Window", "Couldn't set dpi awareness.");
  }

  hWnd = CreateWindowEx(NULL, classname.c_str(), windowname.c_str(), WS_OVERLAPPEDWINDOW, 100, 300, wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, params.m_hInstance, NULL);
  if (hWnd == NULL)
  {
    printf("wtf window null\n");
  }
  SetWindowLongPtr(hWnd, GWLP_USERDATA, (size_t)this);

  std::string lol = (windowname + "class");
  m_window = std::make_shared<WindowInternal>(hWnd,params.m_hInstance, wc, params, lol);


  // clean messages away
  simpleReadMessages(-1);
#endif
}

bool Window::open()
{
#if defined(PLATFORM_WINDOWS)
	ShowWindow(m_window->hWnd, m_window->m_params.m_nCmdShow);
#endif
	return true;
}

void Window::close()
{
#if defined(PLATFORM_WINDOWS)
  CloseWindow(m_window->hWnd);
#endif
}

void Window::cursorHidden(bool enabled)
{
#if defined(PLATFORM_WINDOWS)
  ShowCursor(enabled);
#endif
}

bool Window::simpleReadMessages(int64_t frame)
{
  m_frame = frame;
  m_inputs.setFrame(frame);
#if defined(PLATFORM_WINDOWS)
  MSG msg;

  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
  {
    if (msg.message == WM_QUIT)
      return true;

    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return false;
#else
  return true;
#endif
}

