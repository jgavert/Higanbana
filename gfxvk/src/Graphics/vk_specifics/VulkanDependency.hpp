#pragma once
#include "VulkanBuffer.hpp"
#include "VulkanTexture.hpp"
#include "core/src/datastructures/proxy.hpp"

#include <deque>
#include <vector>
#include <vulkan/vulkan.hpp>



struct BufferDependency
{
	int64_t uniqueId;
	vk::Buffer buffer;
	vk::DeviceSize offset;
	vk::DeviceSize range;
	std::shared_ptr<VulkanBufferState> state;
};

class DependencyTracker
{
public:
	enum class DrawType
	{
		BufferCopy,
		Dispatch,
		PrepareForPresent
	};

	std::string drawTypeToString(DrawType type)
	{
		switch (type)
		{
		case DrawType::BufferCopy:
			return "BufferCopy";
		case DrawType::Dispatch:
			return "Dispatch";
		default:
			return "Unknown";
		}
	}
private:
	using DrawCallIndex = int;
	using ResourceUniqueId = int64_t;

	enum class UsageHint
	{
		unknown,
		read,
		write
	};

	struct DependencyPacket
	{
		DrawCallIndex drawIndex;
		ResourceUniqueId resource;
		UsageHint hint;
		vk::AccessFlags access;
	};

	// general info needed
	std::vector<DrawType> m_drawCallInfo;
	std::vector<vk::PipelineStageFlags> m_drawCallStage;
	faze::unordered_map<ResourceUniqueId, DrawCallIndex> m_lastReferenceToResource;
	// std::unordered_map<ResourceUniqueId, DrawCallIndex> m_writeRes; // This could be vector of all writes
	faze::unordered_set<ResourceUniqueId> m_uniqueResourcesThisChain;
	faze::unordered_map<ResourceUniqueId, BufferDependency> m_bufferStates;
	size_t drawCallsAdded = 0;

	// actual jobs used to generate DAG
	std::vector<DependencyPacket> m_jobs;


	// results
	struct ScheduleNode
	{
		DrawCallIndex jobID;
		DrawCallIndex dependency;
	};
	std::vector<ScheduleNode> m_schedulingResult;


	// barriers
	std::vector<vk::BufferMemoryBarrier> aaargh;
	std::vector<int> m_barrierOffsets;

	// caches
	struct WriteCall
	{
		ResourceUniqueId resource;
		DrawCallIndex draw;
	};
	std::vector<WriteCall> m_cacheWrites;
	std::vector<int> m_readRes;

	struct SmallResource
	{
		vk::Buffer buffer;
		vk::AccessFlags flags;
	};
	faze::unordered_map<ResourceUniqueId, SmallResource> m_cache;
public:
	DependencyTracker() {}
	~DependencyTracker()
	{

	}

	void addDrawCall(int drawCallIndex, DrawType name, vk::PipelineStageFlags baseFlags);

	// buffers
	void addReadBuffer(int drawCallIndex, VulkanBufferShaderView& buffer, vk::AccessFlags flags);
	void addModifyBuffer(int drawCallIndex, VulkanBufferShaderView& buffer, vk::AccessFlags flags);

	void addReadBuffer(int drawCallIndex, VulkanBuffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, vk::AccessFlags flags);
	void addModifyBuffer(int drawCallIndex, VulkanBuffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, vk::AccessFlags flags);
	// textures

	void addReadTexture(int drawCallIndex, VulkanTexture& texture, vk::AccessFlags flags, vk::ImageLayout targetLayout);

	// only builds the graph of dependencies.
	void resolveGraph();
	void printStuff(std::function<void(std::string)> func);
	void makeAllBarriers();
	void runBarrier(vk::CommandBuffer gfx, int nextDrawCall);
	void reset();
};
