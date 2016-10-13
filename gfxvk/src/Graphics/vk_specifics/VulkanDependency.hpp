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
	//vk::DeviceSize offset;
	//vk::DeviceSize range;
	std::shared_ptr<VulkanBufferState> state;
};

struct TextureDependency
{
  int64_t uniqueId;
  vk::Image texture;
  //vk::ImageSubresourceRange range; // these should be per resource
  std::shared_ptr<VulkanImageState> state;
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

  enum class ResourceType
  {
    buffer,
    texture
  };

	struct DependencyPacket
	{
		DrawCallIndex drawIndex;
		ResourceUniqueId resource;
		UsageHint hint;
    ResourceType type;
		vk::AccessFlags access;
    vk::ImageLayout layout;
    uint64_t custom1; // for buffer, "offset". for texture, "miplevel"
    uint64_t custom2; // for buffer, "range" . for texture, "arraylevel"
	};

	// general info needed
	std::vector<DrawType> m_drawCallInfo;
	std::vector<vk::PipelineStageFlags> m_drawCallStage;
	faze::unordered_map<ResourceUniqueId, DrawCallIndex> m_lastReferenceToResource;
	// std::unordered_map<ResourceUniqueId, DrawCallIndex> m_writeRes; // This could be vector of all writes
	faze::unordered_set<ResourceUniqueId> m_uniqueBuffersThisChain;
	faze::unordered_map<ResourceUniqueId, BufferDependency> m_bufferStates;
	size_t drawCallsAdded = 0;

  //needed for textures
	faze::unordered_set<ResourceUniqueId> m_uniqueTexturesThisChain;
	faze::unordered_map<ResourceUniqueId, TextureDependency> m_textureStates;



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

	std::vector<vk::ImageMemoryBarrier> imageBarriers;
	std::vector<int> m_imageBarrierOffsets;

	// caches
	struct WriteCall
	{
		ResourceUniqueId resource;
		DrawCallIndex draw;
	};
	std::vector<WriteCall> m_cacheWrites;
	std::vector<int> m_readRes;

	struct SmallBuffer
	{
		vk::Buffer buffer;
		vk::AccessFlags flags;
	};

  struct SmallTexture
  {
    vk::Image image;
    vk::AccessFlags flags;
    vk::ImageLayout layout;
  };

	faze::unordered_map<ResourceUniqueId, SmallBuffer> m_bufferCache;
	faze::unordered_map<ResourceUniqueId, SmallTexture> m_imageCache;
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
  void addReadBuffer(int drawCallIndex, VulkanTextureShaderView& texture, vk::AccessFlags flags);
  void addModifyBuffer(int drawCallIndex, VulkanTextureShaderView& texture, vk::AccessFlags flags);

	void addReadTexture(int drawCallIndex, VulkanTexture& texture, vk::AccessFlags flags, vk::ImageLayout targetLayout, uint64_t miplevel = 0, uint64_t arraylevel = 0);
  void addWriteTexture(int drawCallIndex, VulkanTexture& texture, vk::AccessFlags flags, vk::ImageLayout layoutUsedInView, uint64_t miplevel = 0, uint64_t arraylevel = 0);

	// only builds the graph of dependencies.
	void resolveGraph();
	void printStuff(std::function<void(std::string)> func);
	void makeAllBarriers();
	void runBarrier(vk::CommandBuffer gfx, int nextDrawCall);
	void reset();
};
