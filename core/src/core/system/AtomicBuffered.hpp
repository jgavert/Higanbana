#pragma once

#include <atomic>

namespace faze
{
  template <typename T>
  class AtomicDoubleBuffered
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
}