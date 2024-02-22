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
	virtual void EnableInstanceExtensions(std::vector<const char*>& enabledInstanceExtensions) const;
	virtual void EnablePhysicalDeviceFeatures(VkPhysicalDeviceFeatures& enabledFeatures) const;
	virtual void EnablePhysicalDeviceExtentions(std::vector<const char*> enabledDeviceExtensions) const;

	virtual bool Init(HINSTANCE hInstance, const CommandLineOptions& options);

	bool IsExtensionSupported(const std::string& extension) const;
private:
	bool InitVulkan(bool enableValidation, std::optional<uint32_t> preferedGPUIdx, bool listDevices);
	HWND SetupWindow(HINSTANCE hInstance, uint32_t width, uint32_t height, bool fullscreen);

	bool CreateVulkanInstance(bool enableValidation);
	bool CreateVulkanDevice(const VkPhysicalDeviceFeatures& enabledFeatures,
							const std::vector<const char*>& enabledExtensions,
							VkQueueFlags requestedQueueTypes);
	VkCommandPool CreateCommandPool(uint32_t queueFamilyIndex,
		VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	uint32_t GetQueueFamilyIndex(const std::vector<VkQueueFamilyProperties>& queueFamilyProperties, VkQueueFlags queueFlags) const;
private:
	VkInstance m_vkInstance;
	VkPhysicalDevice m_vkPhysicalDevice;
	// Vk logical device
	VkDevice m_vkDevice;
	// Handle to the command pool for allocating graphics command buffers 
	VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE;
	// Handle to the device graphics queue that command buffers are submitted to
	VkQueue m_graphicsQueue{ VK_NULL_HANDLE };
	// Stores physical device properties (for e.g. checking device limits)
	VkPhysicalDeviceProperties m_deviceProperties{};
	// Stores the features available on the selected physical device (for e.g. checking if a feature is available)
	VkPhysicalDeviceFeatures m_deviceFeatures{};
	// Stores all available memory (type) properties for the physical device
	VkPhysicalDeviceMemoryProperties m_deviceMemoryProperties{};
	// extensions supported by the device
	std::vector<std::string> m_supportedExtensions;
	// contains queue family indices
	struct
	{
		uint32_t graphics;
		uint32_t compute;
		uint32_t transfer;
	} m_queueFamilyIndices;
	
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