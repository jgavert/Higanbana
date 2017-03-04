#include "intermediatelist.hpp"


namespace faze
{
	namespace backend 
	{
		CommandPacket::CommandPacket()
		{}

		void CommandPacket::setNextPacket(CommandPacket* packet)
		{
			m_nextPacket = packet;
		}
		CommandPacket* CommandPacket::nextPacket()
		{
			return m_nextPacket;
		}

		// Average size estimation for a single continuos list is small.
		// These can be linked together to form one big list.

    IntermediateList::IntermediateList(LinearAllocator&& allocator)
      : m_allocator(std::forward<LinearAllocator>(allocator))
    {}

    IntermediateList::~IntermediateList()
    {
      clear();
    }

    void IntermediateList::clear()
    {
      CommandPacket* current = m_firstPacket;
      while (current != nullptr)
      {
        CommandPacket* tmp = (*current).nextPacket();
        (*current).~CommandPacket();
        current = tmp;
      }
      m_allocator.reset();
      m_firstPacket = nullptr;
      m_lastPacket = nullptr;
      m_size = 0;
    }
	}
}
