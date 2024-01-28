#pragma once

#include <vulkan/vulkan.h>
#include <assert.h>
#include <string>

namespace VulkanUtils
{
    // Display error message and exit on fatal error
	void FatalExit(const std::string& message, int32_t exitCode);
	void FatalExit(const std::string& message, VkResult resultCode);

    std::string VkResultToString(VkResult result);
    std::string VkPhysicalDeviceTypeToString(VkPhysicalDeviceType type);
}

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << VulkanUtils:: VkResultToString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}

