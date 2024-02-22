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
{
	vkDestroyCommandPool(m_vkDevice, m_graphicsCommandPool, nullptr);

	vkDestroyDevice(m_vkDevice, nullptr);
	VulkanDebugUtils::FreeDebugCallback(m_vkInstance);
	vkDestroyInstance(m_vkInstance, nullptr);
}

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

bool VulkanAppBase::CreateVulkanInstance(bool enableValidation)
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

uint32_t VulkanAppBase::GetQueueFamilyIndex(const std::vector<VkQueueFamilyProperties>& queueFamilyProperties, VkQueueFlags queueFlags) const
{
	// Dedicated queue for compute
	// Try to find a queue family index that supports compute but not graphics
	if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
		{
			if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
				((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
			{
				return i;
			}
		}
	}

	// Dedicated queue for transfer
	// Try to find a queue family index that supports transfer but not graphics and compute
	if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
		{
			if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
				((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
				((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
			{
				return i;
			}
		}
	}

	// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
	for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
	{
		if ((queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags)
		{
			return i;
		}
	}

	VulkanUtils::FatalExit("Could not find a matching queue family index", -1);
	return UINT32_MAX;
}

VkCommandPool VulkanAppBase::CreateCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags)
{
	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
	cmdPoolInfo.flags = createFlags;
	VkCommandPool cmdPool;
	VK_CHECK_RESULT(vkCreateCommandPool(m_vkDevice, &cmdPoolInfo, nullptr, &cmdPool));
	return cmdPool;
}

bool VulkanAppBase::CreateVulkanDevice(const VkPhysicalDeviceFeatures& enabledFeatures,
	const std::vector<const char*>& enabledExtensions, VkQueueFlags requestedQueueTypes)
{
	// Queue family properties, used for setting up requested queues upon device creation
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, nullptr);
	assert(queueFamilyCount > 0);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;
	queueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

	// Get queue family indices for the requested queue family types
	// Note that the indices may overlap depending on the implementation

	const float defaultQueuePriority(0.0f);

	// Graphics queue
	if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
	{
		m_queueFamilyIndices.graphics = GetQueueFamilyIndex(queueFamilyProperties, VK_QUEUE_GRAPHICS_BIT);
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = m_queueFamilyIndices.graphics;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &defaultQueuePriority;
		queueCreateInfos.push_back(queueInfo);
	}
	else
	{
		m_queueFamilyIndices.graphics = 0;
	}

	// Dedicated compute queue
	if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
	{
		m_queueFamilyIndices.compute = GetQueueFamilyIndex(queueFamilyProperties, VK_QUEUE_COMPUTE_BIT);
		if (m_queueFamilyIndices.compute != m_queueFamilyIndices.graphics)
		{
			// If compute family index differs, we need an additional queue create info for the compute queue
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = m_queueFamilyIndices.compute;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
	}
	else
	{
		// Else we use the same queue
		m_queueFamilyIndices.compute = m_queueFamilyIndices.graphics;
	}

	// Dedicated transfer queue
	if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
	{
		m_queueFamilyIndices.transfer = GetQueueFamilyIndex(queueFamilyProperties, VK_QUEUE_TRANSFER_BIT);
		if ((m_queueFamilyIndices.transfer != m_queueFamilyIndices.graphics) &&
			(m_queueFamilyIndices.transfer != m_queueFamilyIndices.compute))
		{
			// If transfer family index differs, we need an additional queue create info for the transfer queue
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = m_queueFamilyIndices.transfer;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
	}
	else
	{
		// Else we use the same queue
		m_queueFamilyIndices.transfer = m_queueFamilyIndices.graphics;
	}

	// Create the logical device representation
	std::vector<const char*> deviceExtensions(enabledExtensions);
	// App always use swapchain extension
	deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

	if (deviceExtensions.size() > 0)
	{
		for (const char* enabledExtension : deviceExtensions)
		{
			if (!IsExtensionSupported(enabledExtension))
			{
				std::cerr << "Enabled device extension \"" << enabledExtension << "\" is not present at device level\n";
			}
		}

		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	}


	VkResult result = vkCreateDevice(m_vkPhysicalDevice, &deviceCreateInfo, nullptr, &m_vkDevice);
	VK_CHECK_RESULT(result);
	return result == VK_SUCCESS;
}

bool VulkanAppBase::InitVulkan(bool enableValidation, std::optional<uint32_t> preferedGPUIdx, bool listDevices)
{
	if (!CreateVulkanInstance(enableValidation))
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

	CreateVulkanDevice(enabledFeatures, enabledDeviceExtensions, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

	m_graphicsCommandPool = CreateCommandPool(m_queueFamilyIndices.graphics);

	vkGetDeviceQueue(m_vkDevice, m_queueFamilyIndices.graphics, 0, &m_graphicsQueue);

	return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

HWND VulkanAppBase::SetupWindow(HINSTANCE hInstance, uint32_t width, uint32_t height, bool fullscreen)
{
	WNDCLASSEX wndClass;

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = m_appName.c_str();
	wndClass.hIconSm = LoadIcon(nullptr, IDI_WINLOGO);

	if (!RegisterClassEx(&wndClass))
	{
		VulkanUtils::FatalExit("Could not register window class!\n", 1);
	}

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	if (fullscreen)
	{
		if ((width != (uint32_t)screenWidth) && (height != (uint32_t)screenHeight))
		{
			DEVMODE dmScreenSettings;
			memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
			dmScreenSettings.dmSize       = sizeof(dmScreenSettings);
			dmScreenSettings.dmPelsWidth  = width;
			dmScreenSettings.dmPelsHeight = height;
			dmScreenSettings.dmBitsPerPel = 32;
			dmScreenSettings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
			if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			{
				if (MessageBox(nullptr, "Fullscreen Mode not supported!\n Switch to window mode?", "Error", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
				{
					fullscreen = false;
				}
				else
				{
					return nullptr;
				}
			}
			screenWidth = width;
			screenHeight = height;
		}

	}

	DWORD dwExStyle;
	DWORD dwStyle;

	if (fullscreen)
	{
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}

	RECT windowRect;
	windowRect.left = 0L;
	windowRect.top = 0L;
	windowRect.right = fullscreen ? (long)screenWidth : (long)width;
	windowRect.bottom = fullscreen ? (long)screenHeight : (long)height;

	AdjustWindowRectEx(&windowRect, dwStyle, false, dwExStyle);

	HWND window = CreateWindowEx(0,
		m_appName.c_str(),
		m_appName.c_str(),
		dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0,
		0,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	if (!fullscreen)
	{
		// Center on screen
		uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
		uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
		SetWindowPos(window, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}

	if (!window)
	{
		VulkanUtils::FatalExit("Could not create window!\n", 1);
		return nullptr;
	}

	ShowWindow(window, SW_SHOW);
	SetForegroundWindow(window);
	SetFocus(window);

	return window;
} 

bool VulkanAppBase::Init(HINSTANCE hInstance, const CommandLineOptions& options)
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

	bool fullscreen = options.IsSet("fullscreen");
	uint32_t width = options.GetValueAsInt("width", 800);
	uint32_t height = options.GetValueAsInt("height", 600);
	SetupWindow(hInstance, width, height, fullscreen);

	return initResult;
}

bool VulkanAppBase::IsExtensionSupported(const std::string& extension) const
{
	return (std::find(m_supportedExtensions.begin(), m_supportedExtensions.end(), extension) != m_supportedExtensions.end());
}
