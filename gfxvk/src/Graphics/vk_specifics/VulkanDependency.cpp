#include "VulkanDependency.hpp"

void DependencyTracker::addDrawCall(int, DrawType name, vk::PipelineStageFlags baseFlags)
{
	m_drawCallInfo.emplace_back(name);
	m_drawCallStage.emplace_back(baseFlags);
	drawCallsAdded++;
}

void DependencyTracker::addReadBuffer(int drawCallIndex, VulkanBufferShaderView& buffer, vk::AccessFlags flags)
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
	m_uniqueResourcesThisChain.insert(uniqueID);
}
void DependencyTracker::addModifyBuffer(int drawCallIndex, VulkanBufferShaderView& buffer, vk::AccessFlags flags)
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
	m_uniqueResourcesThisChain.insert(uniqueID);
}

void DependencyTracker::addReadBuffer(int drawCallIndex, VulkanBuffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, vk::AccessFlags flags)
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
	m_uniqueResourcesThisChain.insert(uniqueID);
}
void DependencyTracker::addModifyBuffer(int drawCallIndex, VulkanBuffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, vk::AccessFlags flags)
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

	m_uniqueResourcesThisChain.insert(uniqueID);
}

void DependencyTracker::addReadTexture(int , VulkanTexture& texture, vk::AccessFlags , vk::ImageLayout )
{
	// TODO: implement texture tracking and dependencies

  auto uniqueID = texture.uniqueId;

}

// only builds the graph of dependencies.
void DependencyTracker::resolveGraph()
{
	auto currentSize = m_jobs.size();
	// create DAG(directed acyclic graph) from scratch
	m_schedulingResult.clear();
	//m_schedulingResult.reserve(m_jobs.size());

	m_cacheWrites.clear(); // resource producer list

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

	int currentJobId = 0;
	for (int i = 0; i < static_cast<int>(currentSize); ++i)
	{
		auto& obj = m_jobs[i];
		currentJobId = obj.drawIndex;
		// find all resources?
		m_readRes.clear();
		// move 'i' to next object.
		for (; i < static_cast<int>(currentSize); ++i)
		{
			auto& job = m_jobs[i];
			if (job.drawIndex != currentJobId)
				break;
			if (job.hint == UsageHint::read)
			{
				m_readRes.push_back(i);
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
		if (m_readRes.empty()) // doesn't read any results of other jobs in this graph.
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
			for (auto&& it : m_readRes)
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

void DependencyTracker::makeAllBarriers()
{
	int jobsSize = static_cast<int>(m_jobs.size());
	int executeSize = static_cast<int>(m_schedulingResult.size());


	// this function should figure out transitions within a commandbuffer
	// We should use a small cache of currently "live" resouces that will be referenced in future
	// those that won't be, they should be evicted.
	// resources should contain "last drawcall where referenced"

	// so that we can build the barriers needed after first barrier
	// we just need to create barrier for first that should move all relevant resources to their next needed state
	m_cache.clear();

	// fill cache with all resources seen.
	for (auto&& id : m_uniqueResourcesThisChain)
	{
		auto flags = m_bufferStates[id].state->flags;
		if (flags != vk::AccessFlags(vk::AccessFlagBits(0)))
			m_cache[id] = SmallResource{ m_bufferStates[id].buffer, flags };
	}

	int i = 0;

	int barrierOffsets = 0;
	for (int drawId = 0; drawId < executeSize; ++drawId)
	{
		int secondCall = m_schedulingResult[drawId].jobID;

		m_barrierOffsets.emplace_back(barrierOffsets);
		while (i < jobsSize && m_jobs[i].drawIndex == secondCall)
		{
			auto jobResAccess = m_jobs[i].access;
			auto resource = m_cache.find(m_jobs[i].resource);
			if (resource != m_cache.end())
			{
				auto lastAccess = resource->second.flags;
				if (jobResAccess != lastAccess)
				{
					aaargh.emplace_back(vk::BufferMemoryBarrier()
						.setSrcAccessMask(lastAccess)
						.setDstAccessMask(jobResAccess)
						.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
						.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
						.setBuffer(resource->second.buffer)
						.setOffset(0)
						.setSize(VK_WHOLE_SIZE));
					resource->second.flags = jobResAccess;
					++barrierOffsets;
				}
			}
			else // resource that we hadn't seen before
			{
				m_cache[m_jobs[i].resource] = SmallResource{ m_bufferStates[m_jobs[i].resource].buffer, jobResAccess };
			}
			++i;
		}
	}
	m_barrierOffsets.emplace_back(static_cast<int>(aaargh.size()));

	// update global state
	for (auto&& obj : m_cache)
	{
		m_bufferStates[obj.first].state->flags = obj.second.flags;
	}

}

void DependencyTracker::runBarrier(vk::CommandBuffer gfx, int nextDrawCall)
{
	if (nextDrawCall == 0)
	{
		int barrierOffset = 0;
		int barrierSize = (m_barrierOffsets.size() > 1) ? m_barrierOffsets[1] : static_cast<int>(aaargh.size());
		if (barrierSize == 0)
		{
			return;
		}
		vk::PipelineStageFlags last = vk::PipelineStageFlagBits::eAllCommands;
		vk::PipelineStageFlags next = m_drawCallStage[0];
		vk::ArrayProxy<const vk::MemoryBarrier> memory(0, 0);
		vk::ArrayProxy<const vk::BufferMemoryBarrier> buffer(static_cast<uint32_t>(barrierSize), aaargh.data() + barrierOffset);
		vk::ArrayProxy<const vk::ImageMemoryBarrier> image(0, 0);
		gfx.pipelineBarrier(last, next, vk::DependencyFlags(), memory, buffer, image);
		return;
	}

	// after first and second, nextDrawCall == 1
	// so we need barriers from offset 0
	int barrierOffset = m_barrierOffsets[nextDrawCall];
	int barrierSize = m_barrierOffsets[nextDrawCall + 1] - barrierOffset;
	if (barrierSize == 0)
	{
		return;
	}
	vk::PipelineStageFlags last = m_drawCallStage[nextDrawCall - 1];
	vk::PipelineStageFlags next = m_drawCallStage[nextDrawCall];

	vk::ArrayProxy<const vk::MemoryBarrier> memory(0, 0);
	vk::ArrayProxy<const vk::BufferMemoryBarrier> buffer(static_cast<uint32_t>(barrierSize), aaargh.data() + barrierOffset);
	vk::ArrayProxy<const vk::ImageMemoryBarrier> image(0, 0);
	gfx.pipelineBarrier(last, next, vk::DependencyFlags(), memory, buffer, image);
}

void DependencyTracker::reset()
{
	m_drawCallInfo.clear();
	m_drawCallStage.clear();
	m_jobs.clear();
	m_lastReferenceToResource.clear();
	m_schedulingResult.clear();
	m_barrierOffsets.clear();
	aaargh.clear();
	m_uniqueResourcesThisChain.clear();
}


void DependencyTracker::printStuff(std::function<void(std::string)> func)
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
		tmp += drawTypeToString(m_drawCallInfo[it.jobID]);
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
		tmp += drawTypeToString(m_drawCallInfo[it.jobID]);
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

