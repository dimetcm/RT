#include "VulkanApp.h"
#include "Win32Helpers.h"
#include "VulkanUtils.h"

#include <ShellScalingAPI.h>
#include <iostream>
#include <set>
#include <algorithm>
#include <array>

#include <VulkanDebugUtils.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

const std::vector<const char*> validationLayers = {
    validationLayerName
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static std::string GetShadersPath()
{
	return "../../../shaders/";
}

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsAndComputeFamily;
    std::optional<uint32_t> presentFamily;

    bool AreComplete() const {
        return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
			indices.graphicsAndComputeFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.AreComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

VulkanAppBase::VulkanAppBase(const std::string& appName)
	: m_appName(appName)
{}

VulkanAppBase::~VulkanAppBase()
{
	CleanupSwapChain();

	vkDestroyPipeline(m_vkDevice, m_computePipeline, nullptr);

	vkDestroyPipeline(m_vkDevice, m_graphicsPipeline, nullptr);

	vkDestroyDescriptorPool(m_vkDevice, m_descriptorPool, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_vkDevice, m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_vkDevice, m_imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_vkDevice, m_computeFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_vkDevice, m_graphicsInFlightFences[i], nullptr);
		vkDestroyFence(m_vkDevice, m_computeInFlightFences[i], nullptr);
	}

	vkDestroyRenderPass(m_vkDevice, m_renderPass, nullptr);

	vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);

	vkDestroyCommandPool(m_vkDevice, m_commandPool, nullptr);

	vkDestroyDevice(m_vkDevice, nullptr);
	VulkanDebugUtils::FreeDebugCallback(m_vkInstance);
	vkDestroyInstance(m_vkInstance, nullptr);
}

void VulkanAppBase::CleanupSwapChain()
{
	for (VkFramebuffer framebuffer : m_swapChainFramebuffers) 
	{
		vkDestroyFramebuffer(m_vkDevice, framebuffer, nullptr);
	}

	for (VkImageView imageView : m_swapChainImageViews)
	{
		vkDestroyImageView(m_vkDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_vkDevice, m_swapChain, nullptr);
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

bool VulkanAppBase::CreateVulkanInstance(bool enableValidation)
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = m_appName.c_str();
	appInfo.pEngineName = "RT";
	appInfo.apiVersion = VK_API_VERSION_1_0;

	std::vector<const char*> requiredExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	std::vector<std::string> supportedExtensions;

	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	
	std::vector<VkExtensionProperties> extensions(extCount);
	if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
	{
		for (VkExtensionProperties& extension : extensions)
		{
			supportedExtensions.push_back(extension.extensionName);
		}
	}

	for (const char* enabledExtension : requiredExtensions) 
	{
		if (std::find(supportedExtensions.begin(), supportedExtensions.end(), enabledExtension) 
			== supportedExtensions.end())
		{
			std::cerr << "Required instance extension \"" << enabledExtension << "\" is not present at instance level\n";
			return false;
		}
	}
	
	if (enableValidation && 
		std::find(supportedExtensions.begin(), supportedExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
		!= supportedExtensions.end()) 
	{
		requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}


	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();
	instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.pNext = nullptr;

	if (enableValidation)
	{
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
			std::cerr << "Validation layer " << validationLayerName << " not present, validation is disabled";
		}
	}

	VK_CHECK_RESULT_MSG(vkCreateInstance(&instanceCreateInfo, nullptr, &m_vkInstance),
		"Could not create Vulkan instance!");

	if (enableValidation)
	{
		if (std::find(supportedExtensions.begin(), supportedExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
			!= supportedExtensions.end())
		{
			VulkanDebugUtils::Setup(m_vkInstance);
		}
	}

	return true;
}

void VulkanAppBase::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_vkPhysicalDevice, m_surface);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();

	VK_CHECK_RESULT_MSG(vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &m_commandPool),
		"Failed to create command pool!");
}

static bool CheckDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions)
	{
    	requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) 
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

static bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices = FindQueueFamilies(device, surface);

    bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.AreComplete() && extensionsSupported && swapChainAdequate;
}

bool VulkanAppBase::CreateVulkanLogicalDevice(bool enableValidationLayers)
{
	QueueFamilyIndices indices = FindQueueFamilies(m_vkPhysicalDevice, m_surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	VK_CHECK_RESULT_MSG(vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice),
		"Failed to create logical device!");

	vkGetDeviceQueue(m_vkDevice, indices.graphicsAndComputeFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_vkDevice, indices.graphicsAndComputeFamily.value(), 0, &m_computeQueue);
	vkGetDeviceQueue(m_vkDevice, indices.presentFamily.value(), 0, &m_presentQueue);

	return true;
}

bool VulkanAppBase::InitVulkan(HINSTANCE hInstance, bool enableValidation,
	std::optional<uint32_t> preferedGPUIdx, bool listDevices)
{
	if (!CreateVulkanInstance(enableValidation))
	{
		VulkanUtils::FatalExit("Failed to vreate Vk instance", -1);
		return false;
	}

	// If requested, we enable the default validation layers for debugging
	if (enableValidation)
	{
		VulkanDebugUtils::SetupDebugging(m_vkInstance);
	}

	// create surface
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = hInstance;
	surfaceCreateInfo.hwnd = m_hwnd;
	VK_CHECK_RESULT_MSG(vkCreateWin32SurfaceKHR(m_vkInstance, &surfaceCreateInfo, nullptr, &m_surface),
		"Can't create surface");

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
	VK_CHECK_RESULT_MSG(vkEnumeratePhysicalDevices(m_vkInstance, &gpuCount, physicalDevices.data()),
		"Could not enumerate physical devices");

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

		if (!IsDeviceSuitable(physicalDevices[selectedDevice], m_surface))
		{
			VulkanUtils::FatalExit("Specified device is not suitable", -1);
		}
	}
	else
	{
		bool deviceFound = false;
		for (uint32_t i = 0; i < gpuCount; ++i)
		{
			if (IsDeviceSuitable(physicalDevices[i], m_surface) )
			{
				deviceFound = true;
				selectedDevice = i;

				VkPhysicalDeviceProperties deviceProperties;
				vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);

				if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					break;
				}
			}
		}

		if (!deviceFound)
		{
			VulkanUtils::FatalExit("Can't find suitable device", -1);
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

	vkGetPhysicalDeviceFeatures(m_vkPhysicalDevice, &m_deviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &m_deviceMemoryProperties);

	CreateVulkanLogicalDevice(enableValidation);
	CreateCommandPool();

	return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	VulkanAppBase* app = reinterpret_cast<VulkanAppBase*>(GetWindowLongPtr(hWnd, 0));

	switch (uMsg)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		ValidateRect(hWnd, NULL);
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			PostQuitMessage(0);
			break;
		}
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			if (app->m_resizing || wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
			{
				uint32_t destWidth = LOWORD(lParam);
				uint32_t destHeight = HIWORD(lParam);
				//ResizeWindow(destWidth, destHeight);
			}
		}
		break;
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO minMaxInfo = (LPMINMAXINFO)lParam;
		minMaxInfo->ptMinTrackSize.x = 64;
		minMaxInfo->ptMinTrackSize.y = 64;
		break;
	}
	case WM_ENTERSIZEMOVE:
		app->m_resizing = true;
		break;
	case WM_EXITSIZEMOVE:
		app->m_resizing = false;
		break;
	}

	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

HWND VulkanAppBase::SetupWindow(HINSTANCE hInstance, uint32_t width, uint32_t height, bool fullscreen)
{
	WNDCLASSEX wndClass;

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = sizeof(this);
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

	SetWindowLongPtr(window, 0, reinterpret_cast<LONG_PTR>(this));

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

static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const VkSurfaceFormatKHR& format : availableFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return availableFormats[0];
}

static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool enableVSync)
{
	// The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
	// This mode waits for the vertical blank ("v-sync")
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	// If v-sync is not requested, try to find a mailbox mode
	// It's the lowest latency non-tearing present mode available
	if (!enableVSync)
	{
		for (const VkPresentModeKHR& presentMode : availablePresentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
			else if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}
	}

	return swapchainPresentMode;
}

static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, const VkExtent2D& desiredExtent)
{
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
        return surfaceCapabilities.currentExtent;
    } 
	else 
	{
		return { 
			.width = std::clamp(desiredExtent.width,
				surfaceCapabilities.minImageExtent.width,
				surfaceCapabilities.maxImageExtent.width),
			.height = std::clamp(desiredExtent.height,
				surfaceCapabilities.minImageExtent.height,
				surfaceCapabilities.maxImageExtent.height),
		};
    }
}

void VulkanAppBase::CreateSwapChain(HINSTANCE hInstance, uint32_t width, uint32_t height, bool enableVSync)
{
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_vkPhysicalDevice, m_surface);

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes, enableVSync);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, {.width = width, .height = height});

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = FindQueueFamilies(m_vkPhysicalDevice, m_surface);
    uint32_t queueFamilyIndices[] = {indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

	if (indices.graphicsAndComputeFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else 
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	VK_CHECK_RESULT_MSG(vkCreateSwapchainKHR(m_vkDevice, &createInfo, nullptr, &m_swapChain),
		"Failed to create swap chain!");

	vkGetSwapchainImagesKHR(m_vkDevice, m_swapChain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_vkDevice, m_swapChain, &imageCount, m_swapChainImages.data());

	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent = extent;
}

void VulkanAppBase::CreateSwapChainImageViews()
{
	m_swapChainImageViews.resize(m_swapChainImages.size());

	for (size_t i = 0; i < m_swapChainImages.size(); i++) 
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VK_CHECK_RESULT_MSG(vkCreateImageView(m_vkDevice, &createInfo, nullptr, &m_swapChainImageViews[i]),
			"Failed to create image views!");
	}
}

void VulkanAppBase::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VK_CHECK_RESULT_MSG(vkCreateRenderPass(m_vkDevice, &renderPassInfo, nullptr, &m_renderPass),
		"Failed to create render pass!");
}

void VulkanAppBase::CreateGraphicsCommandBuffers()
{
	m_graphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_graphicsCommandBuffers.size());

	VK_CHECK_RESULT_MSG(vkAllocateCommandBuffers(m_vkDevice, &allocInfo, m_graphicsCommandBuffers.data()),
		"Failed to allocate graphics command buffers!");
}

void VulkanAppBase::CreateComputeCommandBuffers()
{
	m_computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_computeCommandBuffers.size());

	VK_CHECK_RESULT_MSG(vkAllocateCommandBuffers(m_vkDevice, &allocInfo, m_computeCommandBuffers.data()),
		"Failed to allocate compute command buffers!");
}

void VulkanAppBase::CreateSyncObjects()
{
	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_graphicsInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		VK_CHECK_RESULT_MSG(vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]),
			"Failed to create image available semaphore!");

		VK_CHECK_RESULT_MSG(vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]),
			"Failed to create render finished semaphore!");

		VK_CHECK_RESULT_MSG(vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &m_graphicsInFlightFences[i]), 
			"Failed to create graphics frame fence!");

		VK_CHECK_RESULT_MSG(vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_computeFinishedSemaphores[i]),
			"Failed to create compute finished semaphore!");

		VK_CHECK_RESULT_MSG(vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &m_computeInFlightFences[i]),
			"Failed to create compute frame fence!");
	}
}

void VulkanAppBase::CreateFrameBuffers()
{
	m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

	for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
		VkImageView attachments[] = {
			m_swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_swapChainExtent.width;
		framebufferInfo.height = m_swapChainExtent.height;
		framebufferInfo.layers = 1;

		VK_CHECK_RESULT_MSG(vkCreateFramebuffer(m_vkDevice, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]),
			"Failed to create framebuffer!");
	}
}

void VulkanAppBase::CreateDescriptorPool()
{	
	std::vector<VkDescriptorPoolSize> poolSizes =
	{
		// vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),			// Compute UBO
		// vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),	// Graphics image samplers
		// vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1),			// Storage image for ray traced image output
		// vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1),			// Storage buffer for the scene primitives
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VK_CHECK_RESULT_MSG(vkCreateDescriptorPool(m_vkDevice, &descriptorPoolInfo, nullptr, &m_descriptorPool),
			"Failed to create descriptor pool!");

}

void VulkanAppBase::CreateGraphicsPipeline()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyState.flags = 0;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo rasterizationState{};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.flags = 0;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState blendAttachmentState{};
	blendAttachmentState.colorWriteMask = 0xf;
	blendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendState{};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &blendAttachmentState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_FALSE;
	depthStencilState.depthWriteEnable = VK_FALSE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	viewportState.flags = 0;

	VkPipelineMultisampleStateCreateInfo multisampleState{};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.flags = 0;

	const std::array<VkDynamicState, 2> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStateEnables.data();
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicState.flags = 0;

	// Display pipeline
	const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages =
	{
		VulkanUtils::CreateShaderStage(m_vkDevice, GetShadersPath() + "texture.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
		VulkanUtils::CreateShaderStage(m_vkDevice, GetShadersPath() + "texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	// Binding 0 : Fragment shader image sampler
	VkDescriptorSetLayoutBinding imageSampler{};
	imageSampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageSampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	imageSampler.binding = 0;
	imageSampler.descriptorCount = 1;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pBindings = &imageSampler;
	descriptorSetLayoutCreateInfo.bindingCount = 1;

	VkDescriptorSetLayout descriptorSetLayout{};
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_vkDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

	VkPipelineLayout pipelineLayout{};
	VK_CHECK_RESULT(vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = m_renderPass;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipelineVertexInputStateCreateInfo emptyInputState{};
	emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	emptyInputState.vertexAttributeDescriptionCount = 0;
	emptyInputState.pVertexAttributeDescriptions = nullptr;
	emptyInputState.vertexBindingDescriptionCount = 0;
	emptyInputState.pVertexBindingDescriptions = nullptr;

	pipelineCreateInfo.pVertexInputState = &emptyInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.renderPass = m_renderPass;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_graphicsPipeline));

	for (auto& shaderStage : shaderStages)
	{
		VulkanUtils::DestroyShaderStage(m_vkDevice, shaderStage);
	}

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;
			
	VK_CHECK_RESULT(vkAllocateDescriptorSets(m_vkDevice, &allocInfo, &m_graphicsDescriptorSet));
		
	vkDestroyDescriptorSetLayout(m_vkDevice, descriptorSetLayout, nullptr);
	vkDestroyPipelineLayout(m_vkDevice, pipelineLayout, nullptr);
}

void VulkanAppBase::CreateComputePipeline()
{
	// Binding 0: Storage image (raytraced output)
	VkDescriptorSetLayoutBinding storageImageBinding{};
	storageImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	storageImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	storageImageBinding.binding = 0;
	storageImageBinding.descriptorCount = 1;

	// Binding 1: Uniform buffer block
	VkDescriptorSetLayoutBinding uniformBufferBinding{};
	uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	uniformBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	uniformBufferBinding.binding = 1;
	uniformBufferBinding.descriptorCount = 1;

	std::array<VkDescriptorSetLayoutBinding, 2> setLayoutBindings = {
		storageImageBinding,
		uniformBufferBinding
	};

	VkDescriptorSetLayoutCreateInfo descriptorLayout{};
	descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayout.pBindings = setLayoutBindings.data();
	descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());

	VkDescriptorSetLayout descriptorSetLayout{};
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_vkDevice, &descriptorLayout, nullptr, &descriptorSetLayout));

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

	VkPipelineLayout pipelineLayout{};
	VK_CHECK_RESULT(vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;
			
	VK_CHECK_RESULT(vkAllocateDescriptorSets(m_vkDevice, &allocInfo, &m_computeDescriptorSet));

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = pipelineLayout;
	computePipelineCreateInfo.flags = 0;
	computePipelineCreateInfo.stage =
		VulkanUtils::CreateShaderStage(m_vkDevice, GetShadersPath() + "raytracing.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

	VK_CHECK_RESULT(vkCreateComputePipelines(m_vkDevice, nullptr, 1, &computePipelineCreateInfo, nullptr, &m_computePipeline));

	VulkanUtils::DestroyShaderStage(m_vkDevice, computePipelineCreateInfo.stage);
	vkDestroyDescriptorSetLayout(m_vkDevice, descriptorSetLayout, nullptr);
	vkDestroyPipelineLayout(m_vkDevice, pipelineLayout, nullptr);
}

void VulkanAppBase::Run()
{
	bool quitMessageReceived = false;
	while (!quitMessageReceived) 
	{
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) 
			{
				quitMessageReceived = true;
				break;
			}
		}
		if (!IsIconic(m_hwnd))
		{
			Update();
		}
	}

	vkDeviceWaitIdle(m_vkDevice);
}

void VulkanAppBase::RecordGraphicsCommandBuffer(uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK_RESULT_MSG(vkBeginCommandBuffer(m_graphicsCommandBuffers[m_currentFrame], &beginInfo),
		"Failed to begin recording command buffer!");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_swapChainExtent;

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(m_graphicsCommandBuffers[m_currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_graphicsCommandBuffers[m_currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_swapChainExtent.width;
	viewport.height = (float)m_swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(m_graphicsCommandBuffers[m_currentFrame], 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapChainExtent;
	vkCmdSetScissor(m_graphicsCommandBuffers[m_currentFrame], 0, 1, &scissor);

	vkCmdDraw(m_graphicsCommandBuffers[m_currentFrame], 3, 1, 0, 0);

	vkCmdEndRenderPass(m_graphicsCommandBuffers[m_currentFrame]);

	VK_CHECK_RESULT_MSG(vkEndCommandBuffer(m_graphicsCommandBuffers[m_currentFrame]),
		"Failed to record command buffer!");
}

void VulkanAppBase::Update()
{
    vkWaitForFences(m_vkDevice, 1, &m_graphicsInFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_vkDevice, 1, &m_graphicsInFlightFences[m_currentFrame]);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(m_vkDevice, m_swapChain, UINT64_MAX,
		m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(m_graphicsCommandBuffers[m_currentFrame], 0);
	RecordGraphicsCommandBuffer(imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_graphicsCommandBuffers[m_currentFrame];

	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VK_CHECK_RESULT_MSG(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_graphicsInFlightFences[m_currentFrame]),
		"Failed to submit draw command buffer!");

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(m_presentQueue, &presentInfo);
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanAppBase::Init(HINSTANCE hInstance, const CommandLineOptions& options)
{
	Win32Helpers::SetupConsole(m_appName);
    SetupDPIAwareness();
	const bool enableValidation = options.IsSet("validation");
	std::optional<uint32_t> preferedGPUIdx;
	if (options.IsSet("gpuidx"))
	{
		preferedGPUIdx = options.GetValueAsInt("gpuidx", 0); 
	}

	bool fullscreen = options.IsSet("fullscreen");
	uint32_t width = options.GetValueAsInt("width", 800);
	uint32_t height = options.GetValueAsInt("height", 600);
	m_hwnd = SetupWindow(hInstance, width, height, fullscreen);

	bool initResult = InitVulkan(hInstance, enableValidation, preferedGPUIdx, options.IsSet("gpulist"));
    if (!initResult)
	{
		VulkanUtils::FatalExit("Failed to init vulkan\n", -1);
	}

	bool enableVSync = options.IsSet("vsync");
	CreateSwapChain(hInstance, width, height, enableVSync);
	CreateSwapChainImageViews();

	CreateRenderPass();
	CreateGraphicsCommandBuffers();
	CreateComputeCommandBuffers();
	CreateSyncObjects();

	CreateDescriptorPool();
	CreateGraphicsPipeline();
	CreateComputePipeline();

	CreateFrameBuffers();
}