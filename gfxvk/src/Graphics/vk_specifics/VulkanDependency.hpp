#pragma once
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "VulkanBuffer.hpp"
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
	std::unordered_map<DrawCallIndex, std::string> m_drawCallInfo;
	std::unordered_map<DrawCallIndex, vk::PipelineStageFlags> m_drawCallStage;
	std::unordered_map<ResourceUniqueId, DrawCallIndex> m_lastReferenceToResource;
	// std::unordered_map<ResourceUniqueId, DrawCallIndex> m_writeRes; // This could be vector of all writes
  std::unordered_set<ResourceUniqueId> m_uniqueResourcesThisChain;
	std::unordered_map<ResourceUniqueId, BufferDependency> m_bufferStates;
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
