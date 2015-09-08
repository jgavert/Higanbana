#include "Window.hpp"

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

// Initializes the window and shows it
Window::Window(ProgramParams params, std::string className)
	:m_params(std::move(params))
  , m_className(className)
{
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = m_params.m_hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
  wc.lpszClassName = m_className.c_str();

	RegisterClassEx(&wc);
}

bool Window::open(std::string windowname,int width, int height)
{
	RECT wr = { 0, 0, width, height };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	hWnd = CreateWindowEx(NULL, m_className.c_str(), windowname.c_str(), WS_OVERLAPPEDWINDOW, 100, 300, wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, m_params.m_hInstance, NULL);
	if (hWnd == NULL)
    {
	    printf("wtf window null\n");
	    return false;
	}
	ShowWindow(hWnd, m_params.m_nCmdShow);
	return true;
}

void cursorHidden(bool enabled)
{
  ShowCursor(enabled);
}
