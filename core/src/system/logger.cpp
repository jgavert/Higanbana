#include "logger.hpp"

using namespace faze;

Logger::Logger() : m_readerIndex(0), m_writerIndex(1), unhandled_buffer_size(0)
{
  buffer.forEach([](logdata& data) {
    data.m_array.fill('\0');
  });
}
Logger::~Logger()
{
  update();
}

void Logger::handleFazMesg(LogMessage &mesg)
{
  std::lock_guard<std::mutex> lk(writeLock);
  if (unhandled_buffer_size >= RINGBUFFERSIZE - 1)
  {
    // force flush
    update();
    return;
  }
  if (m_writerIndex >= RINGBUFFERSIZE)
    m_writerIndex = 0;
  int n = static_cast<int>(strlen(mesg.m_data));
  size_t dataSize = static_cast<size_t>(std::min(n, MAXLOGLENGTH - 1));
  memcpy(&buffer[m_writerIndex].m_array, mesg.m_data, dataSize);
  buffer[m_writerIndex].m_array[dataSize] = 0;
  buffer[m_writerIndex].m_size = dataSize;
  unhandled_buffer_size++; m_writerIndex++;
#ifdef FAZE_PLATFORM_WINDOWS
  //OutputDebugString(mesg.m_data);
#endif
  //std::cerr.write(mesg.m_data, dataSize);
}

void Logger::update()
{
  while (unhandled_buffer_size >= 0)
  {
    if (m_readerIndex >= RINGBUFFERSIZE)
    {
      m_readerIndex = 0;
    }
    //TODO: this has had problems
#ifdef FAZE_PLATFORM_WINDOWS
    OutputDebugStringA(buffer[m_readerIndex].m_array.data());
#endif
    std::cout.write(buffer[m_readerIndex].m_array.data(), buffer[m_readerIndex].m_size);
    unhandled_buffer_size--; m_readerIndex++;
  }
}