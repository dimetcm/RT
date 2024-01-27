#pragma once

#include "CommandLineArgs.h"
#include "CommandLineOptions.h"

#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#include <crtdbg.h>

class VulkanAppBase
{
public:
	VulkanAppBase(const std::string& appName = "VulkanApp");
	virtual ~VulkanAppBase();

	virtual void RegisterCommandLineOptions(CommandLineOptions& options) const;
	virtual void FillInstanceExtensions(std::vector<const char*>& enabledInstanceExtensions) const;
	
	virtual void Init(const CommandLineOptions& options);
private:
	bool InitVulkan(bool enableValidation);
	bool CreateInstance(bool enableValidation);

private:
	std::string m_appName;
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
	app.Init(commandLineOptions);
	return 0;
}