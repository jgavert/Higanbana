#pragma once
#include "vk_specifics/VulkanDescriptorPool.hpp"

#include <memory>

class DescriptorPool
{
private:
	std::shared_ptr<DescriptorPoolImpl> pool;
public:
	DescriptorPool(std::shared_ptr<DescriptorPoolImpl> pool)
		:pool(pool)
	{}

	DescriptorPool() {}

	DescriptorPoolImpl& impl()
	{
		return *pool;
	}
};