#include "VulkanApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    return StartApp<VulkanAppBase>(CommandLineArgs(__argc, __argv));
}