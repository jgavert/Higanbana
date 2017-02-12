#pragma once

#include "Buffer.hpp"
#include "Texture.hpp"

#include <tuple>

class Binding_
{
  friend class ComputeBinding;
  friend class GraphicsBinding;

  void* stuff;
  using ShaderIndex = unsigned;

public:
  Binding_() :stuff(nullptr) {}
  enum class RootType
  {
    CBV,
    SRV,
    UAV,
	  Num32
  };
  Binding_(std::vector<std::tuple<unsigned, RootType, ShaderIndex>> , unsigned int , unsigned int , unsigned int , int , int)
  {
  }

  void UAV(unsigned int index, BufferUAV& buf);
  void SRV(unsigned int index, BufferSRV& buf);
  void CBV(unsigned int index, BufferCBV& buf);
  void rootConstant(unsigned int index, unsigned int value);
};

class ComputeBinding : public Binding_
{
private:

  ComputeBinding(std::vector<std::tuple<unsigned, RootType, ShaderIndex>> input, unsigned int cbvCount, unsigned int srvCount, unsigned int uavCount, int descriptorTableSRVSlot = -1, int descriptorTableUAVslot = -1) :
    Binding_(input, cbvCount, srvCount, uavCount, descriptorTableSRVSlot, descriptorTableUAVslot)
  {}
public:
  ComputeBinding():Binding_()
  {}
};

class GraphicsBinding : public Binding_
{
  friend class GpuDevice;
  friend class GraphicsCmdBuffer;
  friend class ComputeCmdBuffer;

  GraphicsBinding(std::vector<std::tuple<unsigned, RootType, ShaderIndex>> input, unsigned int cbvCount, unsigned int srvCount, unsigned int uavCount, int descriptorTableSRVSlot = -1, int descriptorTableUAVslot = -1) :
    Binding_(input, cbvCount, srvCount, uavCount, descriptorTableSRVSlot, descriptorTableUAVslot)
  {}
public:
  GraphicsBinding() :Binding_()
  {}
};
