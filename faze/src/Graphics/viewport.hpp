#pragma once

class ViewPort
{
private:
  int mViewPort;
public:
  ViewPort(int width, int height);
  int& getDesc();
};
