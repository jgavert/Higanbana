#pragma once
#include "higanbana/core/system/ringbuffer.hpp"
#include "higanbana/core/system/fazmesg.hpp"
#ifdef HIGANBANA_PLATFORM_WINDOWS
#include <Windows.h>
#endif
#include <array>
#include <algorithm>
#include <string.h>
#include <atomic>
#include <mutex>

#define MAXLOGLENGTH 500
#define RINGBUFFERSIZE 300

namespace higanbana
{

  class logdata
  {
  public:
    logdata() :m_size(0) {}
    size_t m_size;
    std::array<char, MAXLOGLENGTH> m_array;
  };

  class Logger : public FazMesg<LogMessage>
  {
  public:
    Logger();
    ~Logger();
    void handleFazMesg(LogMessage &mesg);
    void update();
  private:
    std::mutex writeLock;
    std::atomic<int> m_readerIndex;
    std::atomic<int> m_writerIndex;
    std::atomic<int> unhandled_buffer_size;
    RingBuffer< logdata, RINGBUFFERSIZE> buffer;
  };
}