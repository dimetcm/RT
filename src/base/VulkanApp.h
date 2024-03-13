#pragma once

#include "CommandLineArgs.h"
#include "CommandLineOptions.h"

#include <vulkan/vulkan.h>

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
	bool Init(HINSTANCE hInstance, const CommandLineOptions& options);
private:
	bool InitVulkan(HINSTANCE hInstance, HWND hwnd, bool enableValidation,
		std::optional<uint32_t> preferedGPUIdx, bool listDevices);
	HWND SetupWindow(HINSTANCE hInstance, uint32_t width, uint32_t height, bool fullscreen);
	void CreateSwapChain(HINSTANCE hInstance, HWND hwnd, uint32_t width, uint32_t height, bool enableVSync);

	bool CreateVulkanInstance(bool enableValidation);
	bool CreateVulkanLogicalDevice(bool enableValidationLayers);
	VkCommandPool CreateCommandPool(uint32_t queueFamilyIndex,
		VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
private:
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
	// Surface
	VkSurfaceKHR m_surface;
	// Swap chain
	VkSwapchainKHR m_swapChain;
	
	std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;

	std::string m_appName;
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
	}
	system("pause");
	return 0;
}