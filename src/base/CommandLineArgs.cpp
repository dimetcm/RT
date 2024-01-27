#include "CommandLineArgs.h"

CommandLineArgs::CommandLineArgs() = default;

CommandLineArgs::CommandLineArgs(int argc, char** argv)
{
    for (int i = 0; i < argc; ++i)
    {
        args.push_back(argv[i]);
    }
}
