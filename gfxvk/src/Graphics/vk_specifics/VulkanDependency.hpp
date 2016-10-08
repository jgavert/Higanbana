#pragma once
#include <deque>
#include <unordered_map>
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
	std::unordered_map<ResourceUniqueId, DrawCallIndex> m_writeRes;
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
	void addDrawCall(int drawCallIndex, std::string name, vk::PipelineStageFlags baseFlags)
	{
		m_drawCallInfo[drawCallIndex] = name;
		m_drawCallStage[drawCallIndex] = baseFlags;
		drawCallsAdded++;
	}

	void addReadBuffer(int drawCallIndex, VulkanBufferShaderView& buffer, vk::AccessFlags flags)
	{
		auto uniqueID = buffer.uniqueId;
		m_lastReferenceToResource[uniqueID] = drawCallIndex;
		m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::read, flags });
		if (m_bufferStates.find(uniqueID) == m_bufferStates.end())
		{
			BufferDependency d;
			d.uniqueId = buffer.uniqueId;
			d.buffer = buffer.m_info.buffer;
			d.offset = buffer.m_info.offset;
			d.range = buffer.m_info.range;
			d.state = buffer.m_state;
			m_bufferStates[uniqueID] = std::move(d);
		}
	}
	void addModifyBuffer(int drawCallIndex, VulkanBufferShaderView& buffer, vk::AccessFlags flags)
	{
		auto uniqueID = buffer.uniqueId;
		m_lastReferenceToResource[uniqueID] = drawCallIndex;
		m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::write, flags });
		if (m_bufferStates.find(uniqueID) == m_bufferStates.end())
		{
			BufferDependency d;
			d.uniqueId = buffer.uniqueId;
			d.buffer = buffer.m_info.buffer;
			d.offset = buffer.m_info.offset;
			d.range = buffer.m_info.range;
			d.state = buffer.m_state;
			m_bufferStates[uniqueID] = std::move(d);
		}
	}

	void addReadBuffer(int drawCallIndex, VulkanBuffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, vk::AccessFlags flags)
	{
		auto uniqueID = buffer.uniqueId;
		m_lastReferenceToResource[uniqueID] = drawCallIndex;
		m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::read, flags });
		if (m_bufferStates.find(uniqueID) == m_bufferStates.end())
		{
			BufferDependency d;
			d.uniqueId = buffer.uniqueId;
			d.buffer = *buffer.m_resource;
			d.offset = offset;
			d.range = range;
			d.state = buffer.m_state;
			m_bufferStates[uniqueID] = std::move(d);
		}
	}
	void addModifyBuffer(int drawCallIndex, VulkanBuffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, vk::AccessFlags flags)
	{
		auto uniqueID = buffer.uniqueId;
		m_lastReferenceToResource[uniqueID] = drawCallIndex;
		m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::write, flags });
		if (m_bufferStates.find(uniqueID) == m_bufferStates.end())
		{
			BufferDependency d;
			d.uniqueId = buffer.uniqueId;
			d.buffer = *buffer.m_resource;
			d.offset = offset;
			d.range = range;
			d.state = buffer.m_state;
			m_bufferStates[uniqueID] = std::move(d);
		}
	}

	// only builds the graph of dependencies.
	void resolveGraph()
	{
		auto currentSize = m_jobs.size();
		// create DAG(directed acyclic graph) from scratch
		m_schedulingResult.clear();
		m_schedulingResult.reserve(m_jobs.size());

		struct WriteCall
		{
			ResourceUniqueId resource;
			DrawCallIndex draw;
		};

		std::vector<WriteCall> m_cacheWrites; // resource producer list

		auto cacheContains = [&](ResourceUniqueId id) -> DrawCallIndex
		{
			auto found = std::find_if(m_cacheWrites.begin(), m_cacheWrites.end(), [&](const WriteCall& obj)
			{
				return obj.resource == id;
			});
			if (found != m_cacheWrites.end())
			{
				return found->draw;
			}
			return -1;
		};

		auto cacheInsertOrReplace = [&](ResourceUniqueId id, DrawCallIndex draw)
		{
			auto found = std::find_if(m_cacheWrites.begin(), m_cacheWrites.end(), [&](const WriteCall& obj)
			{
				return obj.resource == id;
			});
			if (found != m_cacheWrites.end())
			{
				found->draw = draw;
			}
			else
			{
				m_cacheWrites.emplace_back(WriteCall{ id, draw });
			}
		};

		std::vector<int> readRes;
		int currentJobId = 0;
		for (int i = 0; i < static_cast<int>(currentSize); ++i)
		{
			auto& obj = m_jobs[i];
			currentJobId = obj.drawIndex;
			// find all resources?
			readRes.clear();
			// move 'i' to next object.
			for (; i < static_cast<int>(currentSize); ++i)
			{
				auto& job = m_jobs[i];
				if (job.drawIndex != currentJobId)
					break;
				if (job.hint == UsageHint::read)
				{
					readRes.push_back(i);
				}
				if (job.hint == UsageHint::write)
				{
					cacheInsertOrReplace(job.resource, job.drawIndex);
				}
			}
			--i; // backoff one, loop exits with extra appended value;

			ScheduleNode n;
			n.jobID = currentJobId;
			n.dependency = -1;
			if (readRes.empty()) // doesn't read any results of other jobs in this graph.
			{
				// which means its fine to be run
				m_schedulingResult.emplace_back(n);
			}
			else
			{
				// has read dependencies;
				// need to search which jobs produce our dependency
				// this is purely optional
				// reading resources is valid even if nobody produces them
				bool foundEvenOne = false;
				for (auto&& it : readRes)
				{
					auto& readr = m_jobs[it].resource;
					auto found = cacheContains(readr);
					if (found != -1)
					{
						foundEvenOne = true;
						n.dependency = found;
						m_schedulingResult.emplace_back(n);
					}
				}
				if (!foundEvenOne)
					m_schedulingResult.emplace_back(n);
			}
		}

		// order is based on insert order
		drawCallsAdded = 0;
	}

	void printStuff(std::function<void(std::string)> func)
	{
		// print graph in some way
		func("// Dependency Graph\n");

		// create dot format output
		std::string tmp = "\n";

		tmp += "\ndigraph{\nrankdir = LR;\ncompound = true;\nnode[shape = record];\n";

		// all resources one by one
		int i = 0;

		// the graph, should be on top
		tmp += "// Dependency Graph...\n";
		tmp += "subgraph cluster_";
		tmp += std::to_string(i);
		tmp += " {\nlabel = \"Dependency Graph (DAG)\";\n";
		func(tmp); tmp.clear();
		for (auto&& it : m_schedulingResult)
		{
			tmp += std::to_string(it.jobID);
			tmp += "[label=\"";
			tmp += std::to_string(it.jobID);
			tmp += ". ";
			tmp += m_drawCallInfo[it.jobID];
			tmp += "\"];\n";
			func(tmp); tmp.clear();
		}
		tmp += "}\n";
		tmp += "// edges\n";
		func(tmp); tmp.clear();
		for (auto&& it : m_schedulingResult)
		{
			if (it.dependency == -1) // didn't have any dependencies
				continue;
			tmp += std::to_string(it.dependency);
			tmp += " -> ";
			tmp += std::to_string(it.jobID);
			tmp += ";\n";
			func(tmp); tmp.clear();
		}
		++i;
		tmp += "// Execution order...\n";
		tmp += "subgraph cluster_";
		tmp += std::to_string(i);
		tmp += "{\nlabel = \"Execution order\";\n";
		func(tmp); tmp.clear();
		int fb = 0;
		for (auto&& it : m_schedulingResult)
		{
			tmp += std::to_string(it.jobID + m_jobs.size());
			tmp += "[label=\"";
			tmp += std::to_string(fb);
			tmp += ". ";
			tmp += m_drawCallInfo[it.jobID];
			tmp += "\"];\n";
			func(tmp); tmp.clear();
			fb++;
		}
		tmp += "}\n";
		tmp += "// edges\n";
		func(tmp); tmp.clear();

		auto lastOne = m_schedulingResult[0].jobID;

		for (size_t k = 1; k < m_schedulingResult.size(); ++k)
		{
			tmp += std::to_string(lastOne + m_jobs.size());
			tmp += " -> ";
			tmp += std::to_string(m_schedulingResult[k].jobID + m_jobs.size());
			tmp += ";\n";
			lastOne = m_schedulingResult[k].jobID;
			func(tmp); tmp.clear();
		}

		tmp += "}\n";
		func(tmp);
	}

	void makeAllBarriers()
	{
		int jobsSize = static_cast<int>(m_jobs.size());
		int executeSize = static_cast<int>(m_schedulingResult.size());

		// this function should figure out transitions within a commandbuffer
		// We should use a small cache of currently "live" resouces that will be referenced in future
		// those that won't be, they should be evicted.
		// resources should contain "last drawcall where referenced"

		// so that we can build the barriers needed after first barrier
		// we just need to create barrier for first that should move all relevant resources to their next needed state
		struct SmallResource
		{
			vk::Buffer buffer;
			vk::AccessFlags flags;
		};
		std::unordered_map<ResourceUniqueId, SmallResource> m_cache;

		int firstCall = m_schedulingResult[0].jobID;

		int i = 0;
		while (i < jobsSize && m_jobs[i].drawIndex == firstCall)
		{
			m_cache[m_jobs[i].resource] = SmallResource{ m_bufferStates[m_jobs[i].resource].buffer, m_jobs[i].access };
			++i;
		}

		// now we have states caused by first call

		int barrierOffsets = 0;
		for (int drawId = 1; drawId < executeSize; ++drawId)
		{
			int secondCall = m_schedulingResult[drawId].jobID;

			m_barrierOffsets.emplace_back(barrierOffsets);
			while (i < jobsSize && m_jobs[i].drawIndex == secondCall)
			{
				auto resource = m_cache.find(m_jobs[i].resource);
				if (resource != m_cache.end())
				{
					if (m_jobs[i].access != resource->second.flags)
					{
						aaargh.emplace_back(vk::BufferMemoryBarrier()
							.setSrcAccessMask(resource->second.flags)
							.setDstAccessMask(m_jobs[i].access)
							.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
							.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
							.setBuffer(resource->second.buffer)
							.setOffset(0)
							.setSize(VK_WHOLE_SIZE));
						++barrierOffsets;
					}
				}
				else // resource that we hadn't seen before
				{
					m_cache[m_jobs[i].resource] = SmallResource{ m_bufferStates[m_jobs[i].resource].buffer, m_jobs[i].access };
				}
				++i;
			}
		}
		m_barrierOffsets.emplace_back(static_cast<int>(aaargh.size()));
	}

	void runBarrier(vk::CommandBuffer gfx, int nextDrawCall)
	{
		if (nextDrawCall == 0)
			return;

		// after first and second, nextDrawCall == 1
		// so we need barriers from offset 0
		int barrierOffset = m_barrierOffsets[nextDrawCall - 1];
		int barrierSize = m_barrierOffsets[nextDrawCall] - barrierOffset;

		vk::PipelineStageFlags last = m_drawCallStage[nextDrawCall - 1];
		vk::PipelineStageFlags next = m_drawCallStage[nextDrawCall];

		vk::ArrayProxy<const vk::MemoryBarrier> memory(0, 0);
		vk::ArrayProxy<const vk::BufferMemoryBarrier> buffer(static_cast<uint32_t>(barrierSize), aaargh.data() + barrierOffset);
		vk::ArrayProxy<const vk::ImageMemoryBarrier> image(0, 0);
		gfx.pipelineBarrier(last, next, vk::DependencyFlags(), memory, buffer, image);
	}

};
