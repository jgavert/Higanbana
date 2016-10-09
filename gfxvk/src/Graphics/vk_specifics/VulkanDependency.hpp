#pragma once
#include "VulkanBuffer.hpp"
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
  faze::unordered_map<DrawCallIndex, std::string> m_drawCallInfo;
  faze::unordered_map<DrawCallIndex, vk::PipelineStageFlags> m_drawCallStage;
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
