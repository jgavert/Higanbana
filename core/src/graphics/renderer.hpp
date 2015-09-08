#pragma once

#include "../global_debug.hpp"
#include "../math/vec_templated.hpp"

#ifdef WIN64
#define NOMINMAX
#include <windows.h>
#endif


#include <iostream>
#include <string>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <functional>
#include <memory>

namespace faze
{
  class Renderer {
  public:
    Renderer(int width, int height);
    Renderer(int width, int height, bool debug);
    ~Renderer();

    void render();
    void fullscreen();
    void windowed();
    vec2 getSize();
    ivec2 getSizei();
    bool isVsync();
    void toggleVsync();
  private:
    void initialize();
    void setViewport();
    ivec2 m_monitorsize;
    ivec2 m_customsize;
    ivec2 m_currentsize;
    int WindowHandle;
    bool m_debug;
    bool m_windowed;
    bool m_vsync;
  };
}
