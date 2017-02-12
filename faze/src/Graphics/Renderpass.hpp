#pragma once

#include "faze/src/Graphics/vk_specifics/Renderpass.hpp"

class Renderpass
{
private:
  RenderpassImpl m_impl;
public:
  RenderpassImpl& impl() { return m_impl; }
};

class Subpass
{
private:
  SubpassImpl m_impl;
public:
  SubpassImpl& impl() { return m_impl; }
};
