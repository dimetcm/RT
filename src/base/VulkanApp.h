#pragma once

#include "CommandLineArgs.h"
#include "CommandLineOptions.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#include <crtdbg.h>

#include <optional>

class VulkanAppBase
{
public:
	VulkanAppBase(const std::string& appName = "VulkanApp");
	virtual ~VulkanAppBase();

	virtual void RegisterCommandLineOptions(CommandLineOptions& options) const;

	void Init(HINSTANCE hInstance, const CommandLineOptions& options);
	void Run();
private:
	bool InitVulkan(bool enableValidation,
		std::optional<uint32_t> preferedGPUIdx, bool listDevices);
	HWND SetupWindow(uint32_t width, uint32_t height, bool fullscreen);
	bool CreateSwapChain(uint32_t width, uint32_t height);
	void CreateSwapChainImageViews();

	bool CreateVulkanInstance(bool enableValidation);
	bool CreateVulkanLogicalDevice(bool enableValidationLayers);
	void CreateCommandPool();
	void CreateRenderPass();
    void CreateGraphicsCommandBuffers();
    void CreateComputeCommandBuffers();
	void CreateSyncObjects();
	void CreateDescriptorPool();
	void CreateGraphicsPipeline();
	void CreateComputePipeline();
	void CreateFrameBuffers();
	void CreateComputeShaderRenderTarget();
	void CreateComputeShaderUBO();

	void CleanupSwapChain(VkSwapchainKHR swapChain);

	void ResizeWindow(uint32_t width, uint32_t height);

	void RecordComputeCommandBuffer();
	void RecordGraphicsCommandBuffer(uint32_t imageIndex);

	void Update();

	uint32_t GetDeviceMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
private:
	HWND m_hwnd;
	HINSTANCE m_hInstance;

	bool m_initialized = false;
	bool m_resizing = false;
	bool m_vsyncEnabled = false;

	VkInstance m_vkInstance;
	VkPhysicalDevice m_vkPhysicalDevice;
	// Vk logical device
	VkDevice m_vkDevice;
	// Stores physical device properties (for e.g. checking device limits)
	VkPhysicalDeviceProperties m_deviceProperties{};
	// Stores the features available on the selected physical device (for e.g. checking if a feature is available)
	VkPhysicalDeviceFeatures m_deviceFeatures{};
	// Stores all available memory (type) properties for the physical device
	VkPhysicalDeviceMemoryProperties m_deviceMemoryProperties{};

	VkCommandPool m_commandPool;
	// Surface
	VkSurfaceKHR m_surface;
	// Swap chain
	VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
	
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;

	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;

	VkQueue m_graphicsQueue;
	VkQueue m_computeQueue;
	VkQueue m_presentQueue;

	VkRenderPass m_renderPass;

	std::vector<VkCommandBuffer> m_graphicsCommandBuffers;
    std::vector<VkCommandBuffer> m_computeCommandBuffers;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkSemaphore> m_computeFinishedSemaphores;
    std::vector<VkFence> m_graphicsInFlightFences;
    std::vector<VkFence> m_computeInFlightFences;

	VkDescriptorPool m_descriptorPool;

	VkDescriptorSet m_graphicsDescriptorSet;
	VkDescriptorSet m_computeDescriptorSet;

	struct
	{
		VkImage image;
		VkDescriptorImageInfo descriptor;
		VkDeviceMemory memory;
		uint32_t width;
		uint32_t height;			
	} m_computeTargetTexture;

	VkPipeline m_graphicsPipeline;
	VkPipelineLayout m_graphicsPipelineLayout;
	VkPipeline m_computePipeline;
	VkPipelineLayout m_computePipelineLayout;

	struct ComputeUBO
	{
		std::vector<VkBuffer> vkBuffers;
		std::vector<VkDeviceMemory> vkBuffersMemory;
		std::vector<void*> mappedBuffers;
		struct UniformBuffer
		{
			float aspectRatio = 1.0f;
		} ubo;
	} m_computeUBO;

	uint32_t m_currentFrame = 0;

	std::string m_appName;

	friend LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

template<class T>
requires std::derived_from<T, VulkanAppBase>
int StartApp(HINSTANCE hInstance, const CommandLineArgs& args)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	{
		T app;
		CommandLineOptions commandLineOptions;
		app.RegisterCommandLineOptions(commandLineOptions);
		commandLineOptions.Parse(args);
		app.Init(hInstance, commandLineOptions);
		app.Run();
	}
	system("pause");
	return 0;
}