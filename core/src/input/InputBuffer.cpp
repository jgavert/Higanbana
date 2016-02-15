#include <iostream>
#include "InputBuffer.hpp"

using namespace faze;

InputBuffer::InputBuffer(): m_tail(0), m_head(0),m_frame(0), f_translator([](int i){return i;})
{
  for (int i = 0; i < m_inputs.size(); i++)
  {
    m_inputs[i] = Input();
  }
}

void InputBuffer::insert(int key, int action, int64_t frame)
{
  //std::cerr << "INPUT: " << key << " " << action << " " << frame << std::endl;
  /*
  if (m_inputs[m_tail].key == key && m_inputs[m_tail].action == action && m_inputs[m_tail].time == frame)
  {
    std::cerr << "Tried to add same key more than once.\n";
    return;
  }*/
  m_inputs[m_tail] = Input(key, action, frame);
  ++m_tail;
  if (m_tail >= m_inputs.size())
  {
    m_tail = m_tail%m_inputs.size();
  }
  
}

void InputBuffer::readUntil(std::function<bool(int, int)> func)
{
  int currentHead = m_head;
  for (int i=currentHead; i<currentHead+m_tail; i++)
  {
    m_head++;
    auto& input = m_inputs[i++];
    if (func(f_translator(input.key),input.action))
    {
      return;
    }
  }
}

void InputBuffer::readTill(int64_t /*time*/, std::function<void(int,int, int64_t)> /*func*/)
{
  // TODO: readTill(time, function)
  std::cerr << "InputBuffer: I don't know wtf to do\n";
}

bool InputBuffer::findAndDisableThisFrame(int key, int action)
{
  for (int i = 1; i < static_cast<int>(m_inputs.size()); i++)
  {
    if (m_inputs[m_tail - i].time != m_frame)
    {
      return false;
    }
    if (m_inputs[m_tail - i].key == key && m_inputs[m_tail - i].action == action)
    {
      m_inputs[m_tail - i].key = -2;
      return true;
    }
  }
  return false;
}

bool InputBuffer::isPressedThisFrame(int key, int action)
{
  for (int i = 1; i < static_cast<int>(m_inputs.size()); i++)
  {
    if (m_inputs[m_tail - i].time != m_frame)
    {
      return false;
    }
    else if (m_inputs[m_tail - i].key == key && m_inputs[m_tail - i].action == action && m_inputs[m_tail - i].time == m_frame)
    {
      return true;
    }
  }
  return false;
}
