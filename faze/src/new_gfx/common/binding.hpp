#pragma once

#include "buffer.hpp"
#include "texture.hpp"

namespace faze
{
  template <typename interface>
  class Binding
  {
    constexpr int srvCount = interface::srv == 0 ? 1 : interface::srv;
    constexpr int uavCount = interface::uav == 0 ? 1 : interface::uav;
    constexpr int resCount = interface::srv + interface::uav;

    backend::RawView m_srvs[srvCount];
    backend::RawView m_uavs[uavCount];
    backend::TrackedState m_res[resCount];
  public:
    decltype(interface::constants) constants;

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
      m_res[interface::srv + pos] = res.dependency();
    }
    void uav(int pos, const TextureUAV& res)
    {
      m_uavs[pos] = res.view();
      m_res[interface::srv + pos] = res.dependency();
    }
  };
}