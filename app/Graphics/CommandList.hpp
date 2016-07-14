#pragma once

#include <vector>

enum class PacketType
{
  CopyResource,
  ComputeBinding,
  GfxBinding,
  Dispatch
};

struct cmdCopyResource
{
  int resourceFrom;
  int resourceTo;
};

struct cmdDispatch
{
  int x;
  int y;
  int z;
};

struct cmdComputeBinding
{
  int pipeline;
  int resourceA;
  int resourceB;
  int descHeap;
};

struct cmdGfxBinding
{
  int pipeline;
  int resourceA;
  int resourceB;
  int descHeap;
};

struct PacketHeader
{
  PacketType type;
};

template <typename T>
void addToVec(std::vector<uint8_t>& vec, T&& data)
{
  for (int i = 0; i < sizeof(T); ++i)
  {
    vec.emplace_back(reinterpret_cast<uint8_t*>(&data)[i]);
  }
}

class CommandBuffer
{
private:

  std::vector<uint8_t> m_packets;
public:

  void Dispatch(int x, int y, int z)
  {
    addToVec(m_packets, PacketHeader{ PacketType::Dispatch });
    addToVec(m_packets, cmdDispatch{ x, y, z });
  }

  size_t size()
  {
    return m_packets.size();
  }
};