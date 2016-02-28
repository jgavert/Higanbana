#include "Window.hpp"

#if defined(PLATFORM_WINDOWS)
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_DESTROY:
  {
    PostQuitMessage(0);
    return 0;
  } break;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
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

  hWnd = CreateWindowEx(NULL, classname.c_str(), windowname.c_str(), WS_OVERLAPPEDWINDOW, 100, 300, wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, params.m_hInstance, NULL);
  if (hWnd == NULL)
  {
    printf("wtf window null\n");
  }

  std::string lol = (windowname + "class");
  m_window = std::make_shared<WindowInternal>(hWnd, wc, params, lol);


  // clean messages away
  simpleReadMessages();
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

bool Window::simpleReadMessages()
{
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

