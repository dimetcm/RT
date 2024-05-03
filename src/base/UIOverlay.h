#pragma once

#include <vulkan/vulkan.h>

class UIOverlay
{
public:
    void Init(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkCommandPool pool, VkQueue queue, VkRenderPass renderPass);
    void Deinit(VkDevice logicalDevice);

    void Update(VkPhysicalDevice physicalDevice, VkDevice logicalDevice);
    void Draw(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkCommandBuffer commandBuffer);
    
private:
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;

    VkImage m_fontImage = VK_NULL_HANDLE;
    VkImageView m_fontView = VK_NULL_HANDLE;
    VkDeviceMemory m_fontMemory = VK_NULL_HANDLE;

    VkSampler m_sampler = VK_NULL_HANDLE;


	void* m_mappedVertexData = nullptr;
	void* m_mappedIndexData = nullptr;

    int m_verticesCount = -1;
    int m_indicesCount = -1;
};
