#pragma once
#include <memory>

bool isAligned2(intptr_t ptr, size_t alignment)
{
	return (ptr % static_cast<uintptr_t>(alignment)) == 0;
}

class CommandPacket
{
private:
	CommandPacket* m_nextPacket;
public:
	CommandPacket()
		:m_nextPacket(nullptr)
	{}

	void setNextPacket(CommandPacket* packet)
	{
		m_nextPacket = packet;
	}
	CommandPacket* nextPacket()
	{
		return m_nextPacket;
	}

	virtual void execute() = 0;
	virtual ~CommandPacket() {}
};

class CopyResourcePacket : public CommandPacket
{
public:
	CopyResourcePacket()
		: resourceFrom(0)
		, resourceTo(0)
	{}
	virtual ~CopyResourcePacket() override {}
	virtual void execute()
	{
		F_LOG("super cool CopyResource\n");
	}
	int resourceFrom;
	int resourceTo;
};

class DispatchPacket : public CommandPacket
{
public:
	DispatchPacket(int x, int y, int z)
		: x(x), y(y), z(z)
	{}
	virtual ~DispatchPacket() override {}
	virtual void execute()
	{
		F_LOG("super cool Dispatch %d %d %d \n", x, y, z);
	}
  int x;
  int y;
  int z;
};

class DrawPacket : public CommandPacket
{
public:
	DrawPacket(int VertexCountPerInstance, int InstanceCount, int StartVertexLocation, int StartInstanceLocation)
		: VertexCountPerInstance(VertexCountPerInstance)
		,InstanceCount(InstanceCount)
		,StartVertexLocation(StartVertexLocation)
		,StartInstanceLocation(StartInstanceLocation)
	{}
	virtual void execute()
	{
		F_LOG("super cool Draw\n");
	}
	virtual ~DrawPacket() override {}
	int VertexCountPerInstance;
	int InstanceCount;
	int StartVertexLocation;
	int StartInstanceLocation;
};

class ResourceBindingPacket : public CommandPacket
{
public:
	ResourceBindingPacket()
		: pipeline(0)
		, resourceA(0)
		, resourceB(0)
		, descHeap(0)
	{}
	virtual ~ResourceBindingPacket() override {}
	virtual void execute()
	{
		F_LOG("super cool ResourceBinding\n");
	}
  int pipeline;
  int resourceA;
  int resourceB;
  int descHeap;
};

uintptr_t calcAlignment(uintptr_t size, size_t alignment)
{
	return (size / alignment) * alignment + alignment;
}

class LinearAllocator
{
private:
	std::unique_ptr<uint8_t[]> m_data;
	size_t m_current;

public:
	LinearAllocator(size_t size)
		: m_data(std::make_unique<uint8_t[]>(size))
		, m_current(0)
	{}

	template <typename T, typename... Args>
	T* alloc(Args&&... args)
	{
		auto freeMemory = m_current;
		m_current += sizeof(T);
		uintptr_t ptrPos = reinterpret_cast<intptr_t>( &m_data[freeMemory]);
		bool asd = isAligned2(ptrPos, 16);
		if (!asd)
		{
			ptrPos = calcAlignment(ptrPos, 16);
		}
		T* ptr = new (reinterpret_cast<uint8_t*>(ptrPos)) T(std::forward<Args>(args)...);
		return ptr;
	}
	inline void reset()
	{
		m_current = 0;
	}
};

class CommandList
{
private:
	LinearAllocator m_allocator;
	CommandPacket* m_firstPacket;
	CommandPacket* m_lastPacket;
	size_t m_size;
public:
	CommandList(LinearAllocator&& allocator)
		: m_allocator(std::forward<LinearAllocator>(allocator))
		, m_firstPacket(nullptr)
		, m_lastPacket(nullptr)
		, m_size(0)
	{}

	~CommandList()
	{
		CommandPacket* current = m_firstPacket;
		while (current != nullptr)
		{
			CommandPacket* tmp = (*current).nextPacket();
			(*current).~CommandPacket();
			current = tmp;
		}
	}

	size_t size() { return m_size; }

	template <typename Type, typename... Args>
	Type& insert(Args&&... args)
	{
		Type* ptr = m_allocator.alloc<Type>(std::forward<Args>(args)...);
		if (m_lastPacket != nullptr)
		{
			m_lastPacket->setNextPacket(ptr);
		}
		if (m_firstPacket == nullptr)
		{
			m_firstPacket = ptr;
		}
		m_lastPacket = ptr;
		m_size++;
		return *ptr;
	}

	template <typename Func>
	void foreach(Func f)
	{
		CommandPacket* current = m_firstPacket;
		while (current != nullptr)
		{
			f(*current);
			current = current->nextPacket();
		}
	}
};

class CommandBuffer
{
private:
	CommandList m_list;
public:
	CommandBuffer(LinearAllocator&& allocator)
		: m_list(std::forward<LinearAllocator>(allocator))
	{}

	void Dispatch(int x, int y, int z)
	{
		m_list.insert<DispatchPacket>(x,y,z);
	}

	void ResourceBinding()
	{
		m_list.insert<ResourceBindingPacket>();
	}

	size_t size()
	{
		return m_list.size();
	}

	void execute()
	{
		m_list.foreach([](CommandPacket& packet)
		{
			packet.execute();
		});
	}
};