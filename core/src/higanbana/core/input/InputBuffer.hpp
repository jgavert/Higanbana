#pragma once
#include "higanbana/core/datastructures/proxy.hpp"
#include "higanbana/core/system/ringbuffer.hpp"

#include <stdint.h>

#define BUFFERSIZE 600 

namespace higanbana
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
    InputBuffer(vector<wchar_t> chars);

    void insert(int key, int action, int64_t frame);
    void readUntil(std::function<bool(int, int)> func);
    void readTill(int64_t time, std::function<void(int, int, int64_t)> func);
    bool findAndDisableThisFrame(int key, int action);
    void goThroughNFrames(int frames, std::function<void(Input)> func);
    void goThroughThisFrame(std::function<void(Input)> func);
    bool isPressedWithinNFrames(int frames, int key, int action);
    bool isPressedThisFrame(int key, int action);
    void setF_translator(std::function<int(int)> const &translator)
    {
      f_translator = translator;
    }
    void setFrame(int64_t frame)
    {
      m_frame = frame;
    }
    int64_t frame() const
    {
      return m_frame;
    }
    vector<wchar_t>& charInputs() {
      return m_chars;
    }

  private:
    struct OldestIndex
    {
      int indexes;
      int oldestIndex;
    };
    OldestIndex findOldestIndex(int frames);
    RingBuffer<Input, BUFFERSIZE> m_inputs;
    int m_tail;
    int m_head;
    int64_t m_frame;
    std::function<int(int)> f_translator;
    vector<wchar_t> m_chars;
    // some kind of hashmap to map inputs ENUM -> actual input(probably system only)
  };
}