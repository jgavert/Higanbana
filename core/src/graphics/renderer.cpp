#include "renderer.hpp"

using namespace faze;

Renderer::Renderer(int width, int height) :
m_customsize({ width, height }),
m_currentsize({ width, height }),
m_monitorsize({ width, height }),
m_debug(false),
m_vsync(true)
{
  initialize();
}
Renderer::Renderer(int width, int height, bool debug) :
m_customsize({ width, height }),
m_currentsize({ width, height }),
m_monitorsize({ width, height }),
m_debug(debug),
m_vsync(true)
{
  initialize();
}

void Renderer::initialize()
{

}

Renderer::~Renderer()
{
}

void Renderer::render()
{
}

void Renderer::fullscreen()
{

}

void Renderer::windowed()
{

}

void Renderer::setViewport()
{
}

vec2 Renderer::getSize()
{
  return{ static_cast<float>(m_currentsize.x()),static_cast<float>(m_currentsize.y()) };
}

ivec2 Renderer::getSizei()
{
  return m_currentsize;
}

bool Renderer::isVsync()
{
  return m_vsync;
}
void Renderer::toggleVsync()
{
  m_vsync = !m_vsync;
}
