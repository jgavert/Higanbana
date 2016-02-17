#pragma once
#include "ComPtr.hpp"
#include <d3d12.h>

class ViewPort
{
private:
  D3D12_VIEWPORT mViewPort;
public:
  ViewPort(int width, int height);
  D3D12_VIEWPORT& getDesc();
};
