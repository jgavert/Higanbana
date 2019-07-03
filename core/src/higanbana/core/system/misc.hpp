#pragma once

#include <atomic>
#include <cmath>

template <typename T>
class DoubleBuffered
{
private:
  std::atomic<int> m_writer = 2;
  std::atomic<int> m_reader = 0;
  T objects[3] = {};

public:
  T readValue()
  {
    int readerIndex = m_reader;
    auto val = objects[readerIndex];
    readerIndex = (readerIndex + 1) % 3;
    int writerIndex = m_writer;
    if (readerIndex != writerIndex)
    {
      m_reader = readerIndex;
    }
    return val;
  }

  void writeValue(T value)
  {
    int writeIndex = m_writer;
    objects[writeIndex] = value;

    writeIndex = (writeIndex + 1) % 3;
    int readerIndex = m_reader;
    if (readerIndex != writeIndex)
    {
      m_writer = writeIndex;
    }
  }
};

class DynamicScale
{
private:
  int m_scale = 10;
  float m_frametimeTarget = 16.6667f; // 60fps
  float m_targetThreshold = 1.f; // 1 millisecond

public:
  DynamicScale(int scale = 10, float frametimeTarget = 16.6667f, float threshold = 1.f)
    : m_scale(scale)
    , m_frametimeTarget(frametimeTarget)
    , m_targetThreshold(threshold)
  {}

  int tick(float frametimeInMilli)
  {
    auto howFar = std::fabs((m_frametimeTarget - frametimeInMilli)*10.f);
    if (frametimeInMilli > m_frametimeTarget)
    {
      //HIGAN_LOG("Decreasing scale %d %f %f\n", m_scale, frametimeInMilli, m_frametimeTarget);
      m_scale -= int(howFar * 2);
    }
    else if (frametimeInMilli < m_frametimeTarget - m_targetThreshold)
    {
      m_scale += int(howFar);
    }
    if (m_scale <= 0)
      m_scale = 1;
    return m_scale;
  }
};