#pragma once

struct Dimension
{
  int width;
  int height;
  Dimension() :width(1), height(1) {}
  Dimension(int width, int height) :width(width), height(height) {}
  Dimension(int size) :width(size), height(1) {}
};
