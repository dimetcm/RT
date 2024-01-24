#pragma once

#pragma comment(linker, "/subsystem:windows")
#include <windows.h>

class VulkanAppBase
{
};

#define VULKAN_APP_MAIN()																		    \
VulkanAppBase *vulkanApp;										    								\
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)						\
{																									\
	if (vulkanApp != nullptr)																		\
	{																								\
		/*vulkanApp->handleMessages(hWnd, uMsg, wParam, lParam);*/									\
	}																								\
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));												\
}																									\
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)									\
{											                                                        \
	return 0;																						\
}