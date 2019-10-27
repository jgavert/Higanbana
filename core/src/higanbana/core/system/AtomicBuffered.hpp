#pragma once

#include "higanbana/core/system/bitpacking.hpp"
#include <atomic>

namespace higanbana
{
  template <typename T, int ObjCount = 3>
  class AtomicBuffered
  {
  private:
    std::atomic<int> m_state = 0;
    T objects[ObjCount] = {};

    struct State
    {
      int originalValue;
      int writer;
      int reader;
    };

    State readState()
    {
      auto val = m_state.load();
      State res{};
      res.originalValue = val;
      res.reader = higanbana::unpackValue<int>(val, 0, 16);
      res.writer = higanbana::unpackValue<int>(val, 16, 16);
      return res;
    }

    bool tryWriteState(State state)
    {
      int valueWithReader, valueWithWriter;
      valueWithReader = packValue<int>(state.reader, 0, 16);
      valueWithWriter = packValue<int>(state.writer, 16, 16);
      int nextValue = valueWithReader | valueWithWriter;
      return m_state.compare_exchange_strong(state.originalValue, nextValue);
    }

  public:
    T read()
    {
      // change read value to new one
      // get a view of atomic and try to exchange it
      State val = readState();
      int oldRead = val.reader;
      do
      {
        val = readState();
        val.reader = (val.reader +1) % ObjCount;
        if (val.reader == val.writer || val.reader == oldRead)
        {
          val.reader = (val.reader +1) % ObjCount;
        }
      } while(!tryWriteState(val));

      return objects[val.reader];
    }

    void write(T value)
    {
      State val = readState();
      int oldWrite = val.writer;
      objects[val.writer] = value;

      do
      {
        val = readState();
        val.writer = (val.writer +1) % ObjCount;
        if (val.reader == val.writer || oldWrite == val.writer)
        {
          val.writer = (val.writer +1) % ObjCount;
        }
      } while(!tryWriteState(val));
    }
  };
}