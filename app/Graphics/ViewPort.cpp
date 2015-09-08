#include "viewport.hpp"

ViewPort::ViewPort(int width, int height)
{
  ZeroMemory(&mViewPort, sizeof(D3D12_VIEWPORT));
  mViewPort.TopLeftX = 0;
  mViewPort.TopLeftY = 0;
  mViewPort.Width = static_cast<float>(width);
  mViewPort.Height = static_cast<float>(height);
}
D3D12_VIEWPORT& ViewPort::getDesc()
{
  return mViewPort;
}
