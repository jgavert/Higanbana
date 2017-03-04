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

    IntermediateList::IntermediateList()
    {
    }

    IntermediateList::IntermediateList(size_t size)
      : m_allocator(size)
      , m_activeMemory(0)
    {
      m_memory.emplace_back(std::make_unique<uint8_t[]>(size));
    }

    IntermediateList::IntermediateList(std::unique_ptr<uint8_t[]> memory, size_t size)
      : m_allocator(size)
      , m_activeMemory(0)
    {
      m_memory.emplace_back(std::forward<decltype(memory)>(memory));
    }

    IntermediateList::~IntermediateList()
    {
      clear();
    }

    // only moving allowed... for now. 
    void IntermediateList::append(IntermediateList&& other)
    {
      // sew together the lists.
      m_lastPacket->setNextPacket(*other.begin());
      m_lastPacket = *other.end();
      m_memory.insert(m_memory.end(), std::make_move_iterator(other.m_memory.begin()), std::make_move_iterator(other.m_memory.end()));
      m_size += other.m_size;
      other.m_firstPacket = nullptr;
      other.m_lastPacket = nullptr;
      other.m_size = 0;
      other.m_memory.clear();
      other.m_memory.shrink_to_fit();
      other.m_allocator.reset();
      other.m_allocator.resize(0);
    }

    void IntermediateList::clear()
    {
      CommandPacket* current = m_firstPacket;
      while (current != nullptr)
      {
        CommandPacket* tmp = current->nextPacket();
        current->~CommandPacket();
        current = tmp;
      }
      m_memory.clear();
      m_memory.shrink_to_fit();
      m_allocator.reset();
      m_firstPacket = nullptr;
      m_lastPacket = nullptr;
      m_size = 0;
      m_activeMemory = -1;
    }
	}
}
