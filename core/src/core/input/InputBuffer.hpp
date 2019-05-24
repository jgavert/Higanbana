#pragma once
#include "core/system/ringbuffer.hpp"

#include <stdint.h>

#define BUFFERSIZE 600 

namespace faze
{
  class Input
  {
  public:
    Input() :key(-1), action(0), time(0) {}
    Input(int k, int a, int64_t t) :key(k), action(a), time(t) {}
    int key;
    int action;
    int64_t time; // or frame
  };

  class InputBuffer
  {
  public:
    InputBuffer();

    void insert(int key, int action, int64_t frame);
    void readUntil(std::function<bool(int, int)> func);
    void readTill(int64_t time, std::function<void(int, int, int64_t)> func);
    bool findAndDisableThisFrame(int key, int action);
    void goThroughThisFrame(std::function<void(Input)> func);
    bool isPressedThisFrame(int key, int action);
    void setF_translator(std::function<int(int)> const &translator)
    {
      f_translator = translator;
    }
    void setFrame(int64_t frame)
    {
      m_frame = frame;
    }

  private:
    RingBuffer<Input, BUFFERSIZE> m_inputs;
    int m_tail;
    int m_head;
    int64_t m_frame;
    std::function<int(int)> f_translator;
    // some kind of hashmap to map inputs ENUM -> actual input(probably system only)
  };
}