#pragma once
#include <memory>



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
	virtual void execute()
	{
		F_LOG("super cool CopyResource");
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
	virtual void execute()
	{
		F_LOG("super cool Dispatch");
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
		F_LOG("super cool Draw");
	}
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
	virtual void execute()
	{
		F_LOG("super cool ResourceBinding");
	}
  int pipeline;
  int resourceA;
  int resourceB;
  int descHeap;
};

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

	template <typename T>
	T* alloc()
	{
		// alignment
		auto freeMemory = m_current;
		m_current += sizeof(T);
		return reinterpret_cast<T*>(m_data[freeMemory]);
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
	{}

	~CommandList()
	{
		CommandPacket* current = m_firstPacket;
		while (current != nullptr)
		{
			current->~CommandPacket();
			current = current->nextPacket();
		}
	}

	size_t size() { return m_size; }

	template <typename Type, typename... Args>
	Type& insert(Args&&... args)
	{
		Type* ptr = m_allocator.alloc<Type>();
		*ptr = Type(std::forward<Args>(args)...);
		m_lastPacket->setNextPacket(ptr);
		m_lastPacket = ptr;
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
		//packet.x
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