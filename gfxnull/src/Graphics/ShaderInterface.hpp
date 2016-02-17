#pragma once
#include <vector>

// my structs for the descriptor, pointers OUT vectors IN
// for the sake of my own sanity and NOW RAW POINTERS OR DELETES.
// Ill just get them wrong also they are const pointers in the structs.


class ShaderInterface 
{
private:
  friend class GpuDevice;
  friend class GpuCommandQueue;
  friend class GfxCommandList;
  friend class CptCommandList;
  friend class shaderUtils;

  void* m_rootSig;
  int m_rootDesc;

  static int copyDesc(const int& )
  {
    return 1;
  }

  ShaderInterface(void* rootSig, const int& desc) : m_rootSig(rootSig), m_rootDesc(copyDesc(desc)){}

  bool isCopyOf(const int& )
  {
    return false;
  }

public:
  ShaderInterface():m_rootSig(nullptr)
  {
  }

  bool operator!=(ShaderInterface& compared)
  {
    return m_rootSig != compared.m_rootSig;
  }
  bool operator==(ShaderInterface& compared)
  {
    return m_rootSig == compared.m_rootSig;
  }
  bool valid()
  {
    return true;
  }
};

