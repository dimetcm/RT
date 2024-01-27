#pragma once

#include "CommandLineArgs.h"
#include "CommandLineOptions.h"

#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#include <crtdbg.h>


class VulkanAppBase
{
public:
	VulkanAppBase();
	virtual ~VulkanAppBase();

	virtual void RegisterCommandLineOptions(CommandLineOptions& options) const;
};

template<class T>
requires std::derived_from<T, VulkanAppBase>
int StartApp(const CommandLineArgs& args)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	T app;
	CommandLineOptions commandLineOptions;
	app.RegisterCommandLineOptions(commandLineOptions);
	commandLineOptions.Parse(args);
	return 0;
}