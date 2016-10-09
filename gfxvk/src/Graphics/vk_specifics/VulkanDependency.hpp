#pragma once
#include "VulkanBuffer.hpp"

#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <sparsepp.h>
#include "core/src/spookyhash/SpookyV2.h"

#define USING_SPARSEPP

inline size_t HashMemory(const void * p, size_t sizeBytes)
{
  return size_t(SpookyHash::Hash64(p, sizeBytes, 0));
}
template <typename K>
inline size_t HashKey(const K* key, size_t size)
{
  return HashMemory(key, size);
}

template<typename TKey>
struct Hasher 
{
  std::size_t operator()(const TKey& key) const
  {
    return HashKey(&key, sizeof(TKey));
  }
};


#if defined(USING_SPARSEPP) && defined(_DEBUG)
template <typename key, typename val>
using unordered_map = spp::sparse_hash_map<key, val, Hasher<key>>;

template <typename key>
using unordered_set = spp::sparse_hash_set<key, Hasher<key>>;
#else
template <typename key, typename val>
using unordered_map = std::unordered_map<key, val, Hasher<key>>;

template <typename key>
using unordered_set = std::unordered_set<key, Hasher<key>>;
#endif


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
  unordered_map<DrawCallIndex, std::string> m_drawCallInfo;
  unordered_map<DrawCallIndex, vk::PipelineStageFlags> m_drawCallStage;
  unordered_map<ResourceUniqueId, DrawCallIndex> m_lastReferenceToResource;
	// std::unordered_map<ResourceUniqueId, DrawCallIndex> m_writeRes; // This could be vector of all writes
  unordered_set<ResourceUniqueId> m_uniqueResourcesThisChain;
	unordered_map<ResourceUniqueId, BufferDependency> m_bufferStates;
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
public:
  void addDrawCall(int drawCallIndex, std::string name, vk::PipelineStageFlags baseFlags);

  void addReadBuffer(int drawCallIndex, VulkanBufferShaderView& buffer, vk::AccessFlags flags);
  void addModifyBuffer(int drawCallIndex, VulkanBufferShaderView& buffer, vk::AccessFlags flags);

  void addReadBuffer(int drawCallIndex, VulkanBuffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, vk::AccessFlags flags);
  void addModifyBuffer(int drawCallIndex, VulkanBuffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, vk::AccessFlags flags);

	// only builds the graph of dependencies.
  void resolveGraph();
  void printStuff(std::function<void(std::string)> func);
  void makeAllBarriers();
  void runBarrier(vk::CommandBuffer gfx, int nextDrawCall);
  void reset();
};
