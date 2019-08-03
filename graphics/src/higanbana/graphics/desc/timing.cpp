#include "higanbana/graphics/desc/timing.hpp"
#include <higanbana/core/system/time.hpp>

namespace higanbana
{
    Timestamp::Timestamp()
      : begin(0)
      , end(1)
    {

    }
    Timestamp::Timestamp(uint64_t begin, uint64_t end)
      : begin(begin)
      , end(end)
    {

    }

    void Timestamp::start()
    {
      auto current = HighPrecisionClock::now();
      begin = std::chrono::duration_cast<std::chrono::nanoseconds>(current.time_since_epoch()).count();
    }

    void Timestamp::stop()
    {
      auto current = HighPrecisionClock::now();
      end = std::chrono::duration_cast<std::chrono::nanoseconds>(current.time_since_epoch()).count();
    }

    uint64_t Timestamp::nanoseconds() const
    {
      return end - begin;
    }
};