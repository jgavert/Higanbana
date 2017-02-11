#pragma once

#include "backend.hpp"
#include "core/src/datastructures/proxy.hpp"
#include <string>
namespace faze
{
  enum class DeviceType
  {
    Unknown,
    IntegratedGpu,
    DiscreteGpu,
    VirtualGpu,
    Cpu
  };

  struct GpuInfo
  {
    int id;
    std::string name;
    int64_t memory;
    int vendor;
    DeviceType type;
    bool canPresent;
  };


  namespace backend
  {
    struct SubsystemImpl;

    struct SubsystemData : std::enable_shared_from_this<SubsystemData>
    {
      std::shared_ptr<SubsystemImpl> impl; // who creates this? default constructable?
      const char* appName;
      unsigned appVersion;
      const char* engineName;
      unsigned engineVersion;

      SubsystemData(const char* appName, unsigned appVersion = 1, const char* engineName = "faze", unsigned engineVersion = 1);
      std::string gfxApi();
      vector<GpuInfo> availableGpus();
    };
  }

  struct DeviceData
  {

  };

  struct CommandListData
  {

  };

  struct BufferShaderViewData
  {

  };

  class BufferShaderView 
  {
  private:
    /*
    Buffer m_buffer;
    BufferShaderViewData m_view;
    BufferShaderView()
    {}
    */
  public:
    /*
    Buffer& buffer()
    {
      return m_buffer;
    }

    bool isValid()
    {
      return m_buffer.isValid();
    }

    BufferShaderViewImpl& view()
    {
      return m_view;
    }*/
  };

  class BufferSRV : public BufferShaderView
  {

  };

  class BufferUAV : public BufferShaderView
  {

  };

  class BufferIBV : public BufferShaderView
  {

  };

  class BufferCBV : public BufferShaderView
  {

  };
}