#pragma once

#include <vulkan/vulkan.h>

#include <string>

namespace VulkanUtils
{
    // Display error message and exit on fatal error
	void FatalExit(const std::string& message, int32_t exitCode);
	void FatalExit(const std::string& message, VkResult resultCode);

    std::string VkResultToString(VkResult result);
}