#pragma once

#include "buffer.hpp"
#include "texture.hpp"

namespace faze
{
  template <typename Shader>
  class Binding
  {
    static constexpr int srvCount = Shader::srv == 0 ? 1 : Shader::srv;
    static constexpr int uavCount = Shader::uav == 0 ? 1 : Shader::uav;
    static constexpr int resCount = Shader::srv + Shader::uav;

    backend::RawView m_srvs[srvCount];
    backend::RawView m_uavs[uavCount];
    backend::TrackedState m_res[(resCount == 0) ? 1 : resCount];
  public:
    decltype(Shader::constants) constants;

    void srv(int pos, const BufferSRV& res)
    {
      m_srvs[pos] = res.view();
      m_res[pos] = res.dependency();
    }
    void srv(int pos, const TextureSRV& res)
    {
      m_srvs[pos] = res.view();
      m_res[pos] = res.dependency();
    }
    void uav(int pos, const BufferUAV& res)
    {
      m_uavs[pos] = res.view();
      m_res[Shader::srv + pos] = res.dependency();
    }
    void uav(int pos, const TextureUAV& res)
    {
      m_uavs[pos] = res.view();
      m_res[Shader::srv + pos] = res.dependency();
    }
  };
}