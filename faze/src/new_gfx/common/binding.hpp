#pragma once

#include "buffer.hpp"
#include "texture.hpp"

namespace faze
{
  template <typename Shader>
  class Binding
  {
    static constexpr int srvCount = Shader::srvs() == 0 ? 1 : Shader::srvs();
    static constexpr int uavCount = Shader::uavs() == 0 ? 1 : Shader::uavs();
    static constexpr int resCount = Shader::srvs() + Shader::uavs();

    backend::RawView m_srvs[srvCount];
    backend::RawView m_uavs[uavCount];
    backend::TrackedState m_res[(resCount == 0) ? 1 : resCount];
  public:
    friend class CommandGraphNode;

    decltype(Shader::constants) constants;

  private:

    MemView<backend::TrackedState> bResources()
    {
      return MemView<backend::TrackedState>(m_res, resCount);
    }

    MemView<backend::RawView> bSrvs()
    {
      return MemView<backend::RawView>(m_srvs, Shader::srvs());
    }

    MemView<backend::RawView> bUavs()
    {
      return MemView<backend::RawView>(m_uavs, Shader::uavs());
    }

    MemView<uint8_t> bConstants()
    {
      return MemView<uint8_t>(reinterpret_cast<uint8_t*>(&constants), sizeof(constants));
    }

  public:

    void srv(int pos, const DynamicBufferView& res)
    {
      F_ASSERT(pos < Shader::srvs() && pos >= 0, "Invalid srv");
      m_srvs[pos] = res.view();
      m_res[pos] = backend::TrackedState{ 0,0,0 };
    }
    void srv(int pos, const BufferSRV& res)
    {
      F_ASSERT(pos < Shader::srvs() && pos >= 0, "Invalid srv");
      m_srvs[pos] = res.view();
      m_res[pos] = res.dependency();
    }
    void srv(int pos, const TextureSRV& res)
    {
      F_ASSERT(pos < Shader::srvs() && pos >= 0, "Invalid srv");
      m_srvs[pos] = res.view();
      m_res[pos] = res.dependency();
    }
    void uav(int pos, const BufferUAV& res)
    {
      F_ASSERT(pos < Shader::uavs() && pos >= 0, "Invalid uav");
      m_uavs[pos] = res.view();
      m_res[Shader::srvs() + pos] = res.dependency();
    }
    void uav(int pos, const TextureUAV& res)
    {
      F_ASSERT(pos < Shader::uavs() && pos >= 0, "Invalid uav");
      m_uavs[pos] = res.view();
      m_res[Shader::srvs() + pos] = res.dependency();
    }
  };
}