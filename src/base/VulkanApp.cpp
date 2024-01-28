#include "VulkanApp.h"
#include "Win32Helpers.h"
#include "VulkanUtils.h"

#include <ShellScalingAPI.h>
#include <iostream>

#include <VulkanDebugUtils.h>

VulkanAppBase::VulkanAppBase(const std::string& appName)
	: m_appName(appName)
{}

VulkanAppBase::~VulkanAppBase()
{}

void VulkanAppBase::RegisterCommandLineOptions(CommandLineOptions& options) const
{
    options.Add("help", { "--help" }, false, "Show help");
    options.Add("validation", { "-v", "--validation" }, false, "Enable validation layers");
    options.Add("vsync", { "-vs", "--vsync" }, false, "Enable V-Sync");
    options.Add("fullscreen", { "-f", "--fullscreen" }, false, "Start in fullscreen mode");
    options.Add("width", { "-w", "--width" }, true, "Set window width");
    options.Add("height", { "-h", "--height" }, true, "Set window height");
	options.Add("gpuidx", { "-g", "--gpu" }, 1, "Select GPU to run on");
	options.Add("gpulist", { "-gl", "--listgpus" }, 0, "Display a list of available Vulkan devices");
}

static void SetupDPIAwareness()
{
    typedef HRESULT *(__stdcall *SetProcessDpiAwarenessFunc)(PROCESS_DPI_AWARENESS);

	HMODULE shCore = LoadLibraryA("Shcore.dll");
	if (shCore)
	{
		SetProcessDpiAwarenessFunc setProcessDpiAwareness =
			(SetProcessDpiAwarenessFunc)(GetProcAddress(shCore, "SetProcessDpiAwareness"));

		if (setProcessDpiAwareness != nullptr)
		{
			setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
		}

		FreeLibrary(shCore);
	}
}

void VulkanAppBase::EnableInstanceExtensions(std::vector<const char*>& enabledInstanceExtensions) const
{}

void VulkanAppBase::EnablePhysicalDeviceFeatures(VkPhysicalDeviceFeatures& enabledFeatures) const
{}

void VulkanAppBase::EnablePhysicalDeviceExtentions(std::vector<const char*> enabledDeviceExtensions) const
{}

bool VulkanAppBase::CreateInstance(bool enableValidation)
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = m_appName.c_str();
	appInfo.pEngineName = "RT";
	appInfo.apiVersion = VK_API_VERSION_1_0;

	std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

	// Enable surface extensions depending on os
#if defined(_WIN32)
	instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	// Get extensions supported by the instance and store for later use
	std::vector<std::string> supportedInstanceExtensions;

	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	
	if (extCount > 0)
	{
		std::vector<VkExtensionProperties> extensions(extCount);
		if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
		{
			for (VkExtensionProperties& extension : extensions)
			{
				supportedInstanceExtensions.push_back(extension.extensionName);
			}
		}
	}

	std::vector<const char*> enabledInstanceExtensions;
	EnableInstanceExtensions(enabledInstanceExtensions);

	// Enabled requested instance extensions
	if (enabledInstanceExtensions.size() > 0) 
	{
		for (const char * enabledExtension : enabledInstanceExtensions) 
		{
			// Output message if requested extension is not available
			if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), enabledExtension)
				== supportedInstanceExtensions.end())
			{
				std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level\n";
				return false;
			}
			instanceExtensions.push_back(enabledExtension);
		}
	}

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = NULL;
	instanceCreateInfo.pApplicationInfo = &appInfo;

	// Enable the debug utils extension if available (e.g. when debugging tools are present)
	if (enableValidation) 
	{
		if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
			!= supportedInstanceExtensions.end())
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
	}

	if (instanceExtensions.size() > 0)
	{
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
	}

	if (enableValidation)
	{
		// The VK_LAYER_KHRONOS_validation contains all current validation functionality.
		// Note that on Android this layer requires at least NDK r20
		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

		// Check if this layer is available at instance level
		uint32_t instanceLayerCount;
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
		std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
		bool validationLayerPresent = false;
		for (const VkLayerProperties& layer : instanceLayerProperties)
		{
			if (strcmp(layer.layerName, validationLayerName) == 0)
			{
				validationLayerPresent = true;
				break;
			}
		}

		if (validationLayerPresent)
		{
			instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
			instanceCreateInfo.enabledLayerCount = 1;
		}
		else
		{
			std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
		}
	}

	VkResult vkResult = vkCreateInstance(&instanceCreateInfo, nullptr, &m_vkInstance);
	if (vkResult != VK_SUCCESS)
	{
		VulkanUtils::FatalExit("Could not create Vulkan instance : \n" + VulkanUtils::VkResultToString(vkResult), vkResult);
	}

	// If the debug utils extension is present we set up debug functions, so samples can label objects for debugging
	if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
		!= supportedInstanceExtensions.end())
	{
		VulkanDebugUtils::Setup(m_vkInstance);
	}

	return vkResult == VK_SUCCESS;
}

bool VulkanAppBase::InitVulkan(bool enableValidation, std::optional<uint32_t> preferedGPUIdx, bool listDevices)
{
	if (!CreateInstance(enableValidation))
	{
		VulkanUtils::FatalExit("failed to vreate Vk instance", -1);
		return false;
	}

	// If requested, we enable the default validation layers for debugging
	if (enableValidation)
	{
		VulkanDebugUtils::SetupDebugging(m_vkInstance);
	}

	// Physical device
	uint32_t gpuCount = 0;
	// Get number of available physical devices
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(m_vkInstance, &gpuCount, nullptr));
	if (gpuCount == 0) 
	{
		VulkanUtils::FatalExit("No device with Vulkan support found", -1);
		return false;
	}

	// Enumerate devices
	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	VkResult err = vkEnumeratePhysicalDevices(m_vkInstance, &gpuCount, physicalDevices.data());
	if (err)
	{
		VulkanUtils::FatalExit("Could not enumerate physical devices : \n" + VulkanUtils::VkResultToString(err), err);
		return false;
	}

	// GPU selection

	// Select physical device to be used for the Vulkan app
	// Defaults to the first discrete device unless specified by command line
	uint32_t selectedDevice = 0;

	// GPU selection via an option
	if (preferedGPUIdx)
	{
		uint32_t index = preferedGPUIdx.value();
		if (index > gpuCount - 1) 
		{
			std::cerr << "Selected device index " << index << 
				" is out of range, reverting to device 0 (use -listgpus to show available Vulkan devices)" << "\n";
		}
		else 
		{
			selectedDevice = index;
		}
	}
	else
	{
		for (uint32_t i = 0; i < gpuCount; ++i)
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				selectedDevice = i;
				break;
			}
		}
	}

	if (listDevices)
	{
		std::cout << "Available Vulkan devices" << "\n";
		for (uint32_t i = 0; i < gpuCount; ++i)
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
			std::cout << "Device [" << i << "] : " << deviceProperties.deviceName << std::endl;
			std::cout << " Type: " << VulkanUtils::VkPhysicalDeviceTypeToString(deviceProperties.deviceType) << "\n";
			std::cout << " API: " << (deviceProperties.apiVersion >> 22) << "." <<
				((deviceProperties.apiVersion >> 12) & 0x3ff) << "." << (deviceProperties.apiVersion & 0xfff) << "\n";
		}
	}

	m_vkPhysicalDevice = physicalDevices[selectedDevice];

	// Store properties (including limits), features and memory properties of the physical device (so that app can check against them)
	vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &m_deviceProperties);
	std::cout << "Selected device: " << m_deviceProperties.deviceName << std::endl;

	// Get list of supported extensions
	uint32_t extCount = 0;
	vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, nullptr, &extCount, nullptr);
	if (extCount > 0)
	{
		std::vector<VkExtensionProperties> extensions(extCount);
		if (vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
		{
			for (auto ext : extensions)
			{
				m_supportedExtensions.push_back(ext.extensionName);
			}
		}
	}

	vkGetPhysicalDeviceFeatures(m_vkPhysicalDevice, &m_deviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &m_deviceMemoryProperties);

	VkPhysicalDeviceFeatures enabledFeatures{};
	EnablePhysicalDeviceFeatures(enabledFeatures);

	std::vector<const char*> enabledDeviceExtensions;
	EnablePhysicalDeviceExtentions(enabledDeviceExtensions);

	return true;
}

bool VulkanAppBase::Init(const CommandLineOptions& options)
{
	Win32Helpers::SetupConsole(m_appName);
    SetupDPIAwareness();
	const bool enableValidation = options.IsSet("validation");
	std::optional<uint32_t> preferedGPUIdx;
	if (options.IsSet("gpuidx"))
	{
		preferedGPUIdx = options.GetValueAsInt("gpuidx", 0); 
	}
	bool initResult = InitVulkan(enableValidation, preferedGPUIdx, options.IsSet("gpulist"));
    if (!initResult)
	{
		std::cerr << "Failed to init vulkan\n";
	}
	return initResult;
}

bool VulkanAppBase::IsExtensionSupported(const std::string& extension) const
{
	return (std::find(m_supportedExtensions.begin(), m_supportedExtensions.end(), extension) != m_supportedExtensions.end());
}
