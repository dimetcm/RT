#pragma once

#include <vulkan/vulkan.h>
#include <assert.h>
#include <string>
#include <iostream>

namespace VulkanUtils
{
    // Display error message and exit on fatal error
	void FatalExit(const std::string& message, int32_t exitCode);
	void FatalExit(const std::string& message, VkResult resultCode);

    std::string VkResultToString(VkResult result);
    std::string VkPhysicalDeviceTypeToString(VkPhysicalDeviceType type);

	VkPipelineShaderStageCreateInfo CreateShaderStage(VkDevice device, const std::string& fileName, VkShaderStageFlagBits stage);
	void DestroyShaderStage(VkDevice device, VkPipelineShaderStageCreateInfo shaderStage);

	uint32_t GetDeviceMemoryTypeIndex(VkPhysicalDevice device, uint32_t typeBits, VkMemoryPropertyFlags properties);

	VkBuffer CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
		VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory& bufferMemory);

	void CopyBuffer(VkDevice vkDevice, VkQueue queue, VkCommandPool commandPool,
		VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	VkCommandBuffer CreateCommandeBuffer(VkDevice logicalDevice, VkCommandPool pool, VkCommandBufferLevel level);

	void SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask,
		VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
		VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);

	std::string GetShadersPath();	
}

#define VK_CHECK_RESULT_MSG(f, errorDesciption)															\
{																										\
	do {																								\
		VkResult res = (f);																				\
		if (res != VK_SUCCESS)																			\
		{																								\
			std::cerr << "Fatal : VkResult is \"" << VulkanUtils:: VkResultToString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
			assert(res == VK_SUCCESS);																	\
			VulkanUtils::FatalExit(errorDesciption, res);												\
		}																								\
	} while(false);																						\
}

#define VK_CHECK_RESULT(f)									\
{															\
	VK_CHECK_RESULT_MSG(f, "No error desciption provided")	\
}