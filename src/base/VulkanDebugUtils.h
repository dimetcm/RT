#pragma once

#include <vulkan/vulkan.h>

namespace VulkanDebugUtils
{
    void Setup(VkInstance instance);
    void SetupDebugging(VkInstance instance);
    void FreeDebugCallback(VkInstance instance);
}