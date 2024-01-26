#include "VulkanApp.h"
#include "CommandLineParser.h"
#include <iostream>

#if defined(_WIN32)
// Win32 : Sets up a console window and redirects standard output to it
void SetupConsole(const std::string& title)
{
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    FILE* stream;
    freopen_s(&stream, "CONIN$", "r", stdin);
    freopen_s(&stream, "CONOUT$", "w+", stdout);
    freopen_s(&stream, "CONOUT$", "w+", stderr);
    // Enable flags so we can color the output
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(consoleHandle, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(consoleHandle, dwMode);
    SetConsoleTitle(TEXT(title.c_str()));
}
#endif

CommandLineArgs::CommandLineArgs() = default;

CommandLineArgs::CommandLineArgs(int argc, char** argv)
{
    for (int i = 0; i < argc; ++i)
    {
        args.push_back(argv[i]);
    }

    CommandLineParser commandLineParser;
    commandLineParser.Add("help", { "--help" }, false, "Show help");
    commandLineParser.Add("validation", { "-v", "--validation" }, false, "Enable validation layers");
    commandLineParser.Add("vsync", { "-vs", "--vsync" }, false, "Enable V-Sync");
    commandLineParser.Add("fullscreen", { "-f", "--fullscreen" }, false, "Start in fullscreen mode");
    commandLineParser.Add("width", { "-w", "--width" }, true, "Set window width");
    commandLineParser.Add("height", { "-h", "--height" }, true, "Set window height");

    commandLineParser.Parse(args);
    if (commandLineParser.IsSet("help"))
    {
#if defined(_WIN32)
        SetupConsole("Vulkan example");
#endif
        commandLineParser.PrintHelp();
        std::cin.get();
        exit(0);
    }
}
