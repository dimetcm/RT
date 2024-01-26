#pragma once

#include <vector>
#include <concepts>

#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#include <crtdbg.h>

struct CommandLineArgs
{
	CommandLineArgs();
	CommandLineArgs(int argc, char** argv);

	std::vector<const char*> args;
};

class VulkanAppBase
{
public:
	VulkanAppBase() {}
	int Run() { return 0; } 
};


template<class T>
requires std::derived_from<T, VulkanAppBase>
int StartApp(const CommandLineArgs& args)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	T app;
	return app.Run();	
}